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

	World::World()
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_current_frame(0), m_navi_map(agent_size)
		 ,m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map) { } 

	World::World(const char *file_name) :World() {
		m_level.load(file_name);
		for(int n = 0; n < m_entity_map.size(); n++)
			m_entity_map[n].ptr->m_world = this;
		m_tile_map.printInfo();
		m_map_name = file_name;

//		updateNaviMap(true);
	}
	World::~World() {
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

	void World::addEntity(Entity *entity) {
		DASSERT(entity);
		entity->m_world = this;
		m_entity_map.add(entity);
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

		//TODO: projectiles will have to be (probably) stored in a grid also, because
		// when hiding occluders, everything that overlaps with the occluder should be hidden also
		for(int n = 0; n < (int)m_projectiles.size(); n++)
			m_projectiles[n]->addToRender(renderer);
		for(int n = 0; n < (int)m_impacts.size(); n++)
			m_impacts[n]->addToRender(renderer);
	}

	template <class T>
	void World::handleContainer(vector<std::unique_ptr<T> > &objects, int frame_skip) {
		for(int n = 0; n < (int)objects.size(); n++) {
			T *object = objects[n].get();
			object->think();
			if(object->m_to_be_removed) {
				if(object->m_grid_index != -1)
					m_entity_map.remove(object);
				objects[n--] = std::move(objects.back());
				objects.pop_back();
				continue;
			}

			for(int f = 0; f < frame_skip; f++)
				object->nextFrame();
			//DASSERT(!isColliding(object->boundingBox(), object));
		}
	}

	void World::simulate(double time_diff) {
		PROFILE("World::simulate");

		DASSERT(time_diff > 0.0);
		double max_time_diff = 1.0; //TODO: add warning?
		time_diff = min(time_diff, max_time_diff);

		double current_time = m_last_time + time_diff; //TODO: rozjedzie sie z getTime(), ale czy to jest problem?
		double frame_diff = current_time - m_last_frame_time;
		double frame_time = 1.0 / 15.0, fps = 15.0;
		m_current_time = current_time;
		m_time_delta = time_diff;

		int frame_skip = 0;
		if(frame_diff > frame_time) {
			frame_skip = (int)(frame_diff * fps);
			m_last_frame_time += (double)frame_skip * frame_time;
		}
		m_current_frame += frame_skip;
		if(m_current_frame < 0)
			m_current_frame = 0;
		Tile::setFrameCounter(m_current_frame);

		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &object = m_entity_map[n];
			if(!object.ptr)
				continue;

			object.ptr->think();
			if(object.ptr->m_to_be_removed) {
				m_entity_map.remove(object.ptr);
				continue;
			}
			if(object.flags & (collider_dynamic | collider_dynamic_nv))
				m_entity_map.update(object.ptr);

			for(int f = 0; f < frame_skip; f++)
				object.ptr->nextFrame();
		}


		handleContainer(m_projectiles, frame_skip);
		handleContainer(m_impacts, frame_skip);

		m_last_time = current_time;
	}
	
	Intersection World::trace(const Segment &segment, const Entity *ignore, int flags) const {
		PROFILE("world::trace");
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
	
	void World::spawnProjectile(PProjectile projectile) {
		projectile->m_world = this;
		m_projectiles.push_back(std::move(projectile));
	}
	
	void World::spawnProjectileImpact(PProjectileImpact impact) {
		impact->m_world = this;
		m_impacts.push_back(std::move(impact));
	}

}
