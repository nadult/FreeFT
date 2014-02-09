/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/world.h"
#include "game/projectile.h"
#include "tile_map.h"
#include "sys/xml.h"
#include "sys/profiler.h"
#include "game/tile.h"
#include "gfx/scene_renderer.h"
#include "navi_heightmap.h"
#include <cstdio>

namespace game {

	// TODO: multiple navigation maps (2, 3, 4 at least)
	enum { agent_size = 3 };

	World *World::s_instance = nullptr;

	World::World(Mode mode)
		:m_mode(mode), m_last_anim_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0),
		m_anim_frame(0), m_navi_map(agent_size) ,m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map) {
		if(m_mode == Mode::server)
			m_replication_list.reserve(1024);

		ASSERT(s_instance == nullptr);
		s_instance = this;
	} 

	World::World(Mode mode, const char *file_name) :World(mode) {
		m_level.load(file_name);
		m_tile_map.printInfo();
		m_map_name = file_name;
//		updateNaviMap(true);
	}
	World::~World() {
		s_instance = nullptr;
	}

	void World::updateNaviMap(bool full_recompute) {
		//PROFILE("updateNaviMap");

		if(full_recompute) {
			vector<IBox> bboxes;
			bboxes.reserve(m_tile_map.size() + m_entity_map.size());

			for(int n = 0; n < m_tile_map.size(); n++)
				if(m_tile_map[n].ptr)
					bboxes.push_back((IBox)m_tile_map[n].bbox);
			for(int n = 0; n < m_entity_map.size(); n++)
				if(m_entity_map[n].ptr && m_entity_map[n].flags & collider_static)
					bboxes.push_back(enclosingIBox(m_entity_map[n].ptr->boundingBox()));

			NaviHeightmap heightmap(m_tile_map.dimensions());
			heightmap.update(bboxes);
		//	heightmap.saveLevels();
		//	heightmap.printInfo();

			m_navi_map.update(heightmap);
			m_navi_map.printInfo();
		}

		m_navi_map.removeColliders();

		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &object = m_entity_map[n];
			if(!object.ptr)
				continue;

			if(object.flags & collider_dynamic_nv) {
				const IBox &box = enclosingIBox(object.ptr->boundingBox());
				m_navi_map.addCollider(IRect(box.min.xz(), box.max.xz()));
			}
		}
	}

	void World::updateVisibility(const FBox &bbox) {
		PROFILE("World::updateVisibility");
		if(m_tile_map.occluderMap().updateVisibility(bbox)) {
			m_tile_map.updateVisibility();
			m_entity_map.updateVisibility();
		}
	}

	void World::removeEntity(int entity_id) {
		DASSERT(entity_id >= 0 && entity_id < m_entity_map.size());
		m_entity_map.remove(entity_id);
	}

	void World::addEntity(int index, Entity *entity) {
		DASSERT(entity);
		m_entity_map.add(index, entity);
		replicate(entity);
	}

	int World::addEntity(Entity *entity) {
		DASSERT(entity);
		m_entity_map.add(entity);
		replicate(entity);
		return entity->m_grid_index;
	}

	void World::addToRender(gfx::SceneRenderer &renderer) {
		PROFILE("World::addToRender");

		vector<int> inds;
		inds.reserve(1024);
		m_tile_map.findAll(inds, renderer.targetRect(), collider_all|visibility_flag);
		for(int n = 0; n < (int)inds.size(); n++) {
			const auto &obj = m_tile_map[inds[n]];
			obj.ptr->addToRender(renderer, (int3)obj.bbox.min);
		}

		inds.clear();
		m_entity_map.findAll(inds, renderer.targetRect(), collider_all|visibility_flag);
		for(int n = 0; n < (int)inds.size(); n++) {
			const auto &obj = m_entity_map[inds[n]];
			obj.ptr->addToRender(renderer);
		}
	}

	void World::simulate(double time_diff) {
		PROFILE("World::simulate");
		//TODO: synchronizing time between client/server

		DASSERT(time_diff > 0.0);
		double max_time_diff = 1.0; //TODO: add warning?
		time_diff = min(time_diff, max_time_diff);

		double current_time = m_last_time + time_diff; //TODO: rozjedzie sie z getTime(), ale czy to jest problem?
		double frame_diff = current_time - m_last_anim_frame_time;
		double frame_time = 1.0 / 15.0, fps = 15.0;
		m_current_time = current_time;
		m_time_delta = time_diff;

		int frame_skip = 0;
		if(frame_diff > frame_time) {
			frame_skip = (int)(frame_diff * fps);
			m_last_anim_frame_time += (double)frame_skip * frame_time;
		}
		m_anim_frame += frame_skip;
		if(m_anim_frame < 0)
			m_anim_frame = 0;
		Tile::setFrameCounter(m_anim_frame);

		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &object = m_entity_map[n];
			if(!object.ptr)
				continue;

			object.ptr->think();
			if(object.ptr->m_to_be_removed) {
				replicate(object.ptr);
				m_entity_map.remove(object.ptr);
				continue;
			}
			if(object.flags & (collider_dynamic | collider_dynamic_nv | collider_projectile))
				m_entity_map.update(object.ptr);

			for(int f = 0; f < frame_skip; f++)
				object.ptr->nextFrame();
		}

		m_last_time = current_time;
	}
	
	Intersection World::trace(const Segment &segment, const Entity *ignore, int flags) const {
		PROFILE("World::trace");
		Intersection out;

		if(flags & collider_tiles) {
			pair<int, float> isect = m_tile_map.trace(segment, -1, flags);
			if(isect.first != -1)
				out = Intersection(&m_tile_map[isect.first], isect.second);
		}

		if(flags & collider_entities) {
			pair<int, float> isect = m_entity_map.trace(segment, ignore? ignore->m_grid_index : -1, flags);
			if(isect.first != -1 && isect.second < out.distance())
				out = Intersection(&m_entity_map[isect.first], isect.second);
		}

		return out;
	}

	Intersection World::pixelIntersect(const int2 &screen_pos, int flags) const {
		//PROFILE("world::pixelIntersect");
		Intersection out;
		Ray ray = screenRay(screen_pos);

		if(flags & collider_tiles) {
			int tile_id = m_tile_map.pixelIntersect(screen_pos, flags);
			if(tile_id != -1)
				out = Intersection(&m_tile_map[tile_id], intersection(ray, m_tile_map[tile_id].bbox));
		}

		if(flags & collider_entities) {
			int entity_id = m_entity_map.pixelIntersect(screen_pos, flags);
			if(entity_id != -1) {
				const auto *object = &m_entity_map[entity_id];
				if(out.isEmpty() || drawingOrder(object->bbox, out.boundingBox()) == 1)
					out = Intersection(object, intersection(ray, object->bbox));
			}
		}

		return out;
	}

	bool World::isColliding(const FBox &box, const Entity *ignore, ColliderFlags flags) const {
		//PROFILE("world::isColliding");

		if((flags & collider_tiles))
			if(m_tile_map.findAny(box, flags) != -1)
				return true;

		if(flags & collider_entities)
			if(m_entity_map.findAny(box, ignore? ignore->m_grid_index : -1, flags) != -1)
				return true;

		return false;
	}

	bool World::isInside(const FBox &box) const {
		return m_tile_map.isInside(box);
	}
		
	vector<int3> World::findPath(const int3 &start, const int3 &end) const {
		PROFILE_RARE("world::findPath");
		return m_navi_map.findPath(start, end);
	}
	
	void World::replicate(const Entity *entity) {
		DASSERT(entity && entity->m_grid_index != -1);
		replicate(entity->m_grid_index);
	}

	void World::replicate(int entity_id) {
		if(m_mode == Mode::server)
			m_replication_list.push_back(entity_id);
	}

}
