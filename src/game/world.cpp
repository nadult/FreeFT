/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/world.h"
#include "tile_map.h"
#include "sys/xml.h"
#include "sys/profiler.h"
#include "game/tile.h"
#include "navi_heightmap.h"
#include "audio/device.h"
#include "game/actor.h"
#include <algorithm>

namespace game {

	World::World(const string &file_name, Mode mode, Replicator *replicator)
		:m_mode(mode), m_last_anim_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0),
		m_anim_frame(0), m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map),
	   	m_replicator(replicator) {
		DASSERT(m_mode == Mode::single_player || m_replicator != nullptr);

		m_level.load(file_name.c_str());
		for(int n = 0; n < m_tile_map.size(); n++) {
			//TODO: leave them and use them
			if(m_tile_map[n].ptr->isInvisible())
				m_tile_map.remove(n);
		}

		for(int n = 0; n < m_entity_map.size(); n++) {
			auto &obj = m_entity_map[n];
			if(obj.ptr)
				obj.ptr->hook(this, n);
		}

		int agent_sizes[] = { 3, 4, 7 };
		for(int n = 0; n < COUNTOF(agent_sizes); n++)
			m_navi_maps.emplace_back(agent_sizes[n]);

//		m_tile_map.printInfo();
		m_map_name = file_name;
		updateNaviMap(true);
	}

	World::~World() {
	}
		
	float World::random() {
		return frand();
	}
		
	void World::updateNaviMap(bool full_recompute) {
		PROFILE("updateNaviMap");
			
		//TODO: what about static entities that are added during the game?

		if(full_recompute) {
			vector<IBox> bboxes, blockers;
			bboxes.reserve(m_tile_map.size() + m_entity_map.size());

			for(int n = 0; n < m_tile_map.size(); n++) {
				const Tile *tile = m_tile_map[n].ptr;
				if(!tile || !Flags::test(tile->flags(), Flags::all | Flags::colliding))
					continue;

				bool is_walkable = Flags::test(tile->flags(), Flags::walkable_tile | Flags::wall_tile);
				(is_walkable? bboxes : blockers).push_back((IBox)m_tile_map[n].bbox);
			}


			for(int n = 0; n < m_entity_map.size(); n++)
				if(m_entity_map[n].ptr && Flags::test(m_entity_map[n].flags, Flags::static_entity | Flags::colliding))
					blockers.push_back(enclosingIBox(m_entity_map[n].ptr->boundingBox()));

			NaviHeightmap heightmap(m_tile_map.dimensions());
			heightmap.update(bboxes, blockers);
			//heightmap.saveLevels();
			//heightmap.printInfo();

			for(int m = 0; m < (int)m_navi_maps.size(); m++) {
				m_navi_maps[m].update(heightmap);
				//m_navi_maps[m].printInfo();
			}
		}

		for(int m = 0; m < (int)m_navi_maps.size(); m++) {
			NaviMap &navi_map = m_navi_maps[m];
			navi_map.removeColliders();

			for(int n = 0; n < m_entity_map.size(); n++) {
				auto &object = m_entity_map[n];
				if(!object.ptr)
					continue;

				if(Flags::test(object.flags, Flags::dynamic_entity | Flags::colliding)) {
					const IBox &box = enclosingIBox(object.ptr->boundingBox());
					navi_map.addCollider(box, n);
				}
			}
			navi_map.updateReachability();
		}
	}

	EntityRef World::addEntity(PEntity &&ptr, int index) {
		DASSERT(ptr);
		Entity *entity = ptr.get();
		index = m_entity_map.add(std::move(ptr), index);
		entity->hook(this, index);
		replicate(index);

		return entity->ref();
	}
		
	void World::removeEntity(EntityRef ref) {
		Entity *entity = refEntity(ref);
		if(entity) {
			m_entity_map.remove(ref.index());
			replicate(ref.index());
		}
	}

	void World::simulate(double time_diff) {
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
			if(object.flags & Flags::dynamic_entity)
				m_entity_map.update(n);

			for(int f = 0; f < frame_skip; f++)
				object.ptr->nextFrame();
		}

		for(int n = 0; n < (int)m_replace_list.size(); n++) {
			auto &pair = m_replace_list[n];
			int index = pair.second;
			int old_uid = -1;

			if(index != -1) {
				const Entity *entity = m_entity_map[index].ptr;
				if(entity)
					old_uid = entity->m_unique_id;
				m_entity_map.remove(index);
			}

			if(pair.first.get()) {
				if(old_uid != -1)
					pair.first.get()->m_unique_id = old_uid;
				index = addEntity(std::move(pair.first), index).index();
			}
			replicate(index);
		}
		m_replace_list.clear();

		m_last_time = current_time;
		updateNaviMap(false);
	}
		
	const EntityMap::ObjectDef *World::refEntityDesc(int index) const {
	   	if(index >= 0 && index < m_entity_map.size())
			return &m_entity_map[index];
		return nullptr;
	}

	const TileMap::ObjectDef *World::refTileDesc(int index) const {
	   	if(index >= 0 && index < m_tile_map.size())
			return &m_tile_map[index];
		return nullptr;
	}
	
	const Grid::ObjectDef *World::refDesc(ObjectRef ref) const {
		return ref.isTile()? (const Grid::ObjectDef*)refTileDesc(ref.m_index) :
			ref.isEntity()? (const Grid::ObjectDef*)refEntityDesc(ref.m_index) : nullptr;
	}
	
	const FBox World::refBBox(ObjectRef ref) const {
		if(ref.isTile()) {
		   	if(ref.m_index >= 0 && ref.m_index < m_tile_map.size())
				return m_tile_map[ref.m_index].bbox;
		}
		else {
		   	if(ref.m_index >= 0 && ref.m_index < m_entity_map.size())
				return m_entity_map[ref.m_index].bbox;
		}

		return FBox::empty();
	}

	const Tile *World::refTile(ObjectRef ref) const {
		if(!ref.m_is_entity && ref.m_index >= 0 && ref.m_index < m_tile_map.size())
			return m_tile_map[ref.m_index].ptr;
		return nullptr;
	}
	
	Entity *World::refEntity(int index) {
		if(index >= 0 && index < m_entity_map.size())
			return m_entity_map[index].ptr;
		return nullptr;
	}
	
	Entity *World::refEntity(ObjectRef ref) {
		if(ref.m_is_entity && ref.m_index >= 0 && ref.m_index < m_entity_map.size())
			return m_entity_map[ref.m_index].ptr;
		return nullptr;
	}
	
	Entity *World::refEntity(EntityRef ref) {
		if(ref.m_index < 0 || ref.m_index >= m_entity_map.size())
			return nullptr;

		Entity *entity = m_entity_map[ref.m_index].ptr;
		if(entity && (entity->m_unique_id == ref.m_unique_id))
			return entity;

		return nullptr;
	}
		
	EntityRef World::toEntityRef(ObjectRef ref) const {
		Entity *entity = const_cast<World*>(this)->refEntity(ref);
		return entity? entity->ref() : EntityRef();
	}
	
	EntityRef World::toEntityRef(int index) const {
		Entity *entity = const_cast<World*>(this)->refEntity(index);
		return entity? entity->ref() : EntityRef();
	}
		
	int World::filterIgnoreIndex(const FindFilter &filter) const {
		if(filter.m_ignore_entity_ref.isValid()) {
			if(const_cast<World*>(this)->refEntity(filter.m_ignore_entity_ref))
				return filter.m_ignore_entity_ref.m_index;
		}
		
		return filter.m_ignore_object_ref.m_is_entity? filter.m_ignore_object_ref.m_index : -1;
	}

	Intersection World::trace(const Segment &segment, const FindFilter &filter) const {
		Intersection out;

		if(filter.m_flags & Flags::tile) {
			pair<int, float> isect = m_tile_map.trace(segment, -1, filter.m_flags);
			if(isect.first != -1)
				out = Intersection(ObjectRef(isect.first, false), isect.second);
		}

		if(filter.m_flags & Flags::entity) {
			int ignore_index = filterIgnoreIndex(filter);

			pair<int, float> isect = m_entity_map.trace(segment, ignore_index, filter.m_flags);
			if(isect.first != -1 && isect.second <= out.distance())
				out = Intersection(ObjectRef(isect.first, true), isect.second);
		}

		return out;
	}
		
	void World::traceCoherent(const vector<Segment> &segments, vector<Intersection> &out, const FindFilter &filter) const {
		out.resize(segments.size());
		vector<pair<int, float>> results;

		if(filter.m_flags & Flags::tile) {
			m_tile_map.traceCoherent(segments, results, -1, filter.m_flags);
			for(int n = 0; n < (int)out.size(); n++)
				if(results[n].first != -1)
					out[n] = Intersection(ObjectRef(results[n].first, false), results[n].second);
		}

		if(filter.m_flags & Flags::entity) {
			int ignore_index = filterIgnoreIndex(filter);

			m_entity_map.traceCoherent(segments, results, ignore_index, filter.m_flags);
			for(int n = 0; n < (int)out.size(); n++)
				if(results[n].first != -1 && results[n].second <= out[n].distance())
					out[n] = Intersection(ObjectRef(results[n].first, true), results[n].second);
		}
	}

	ObjectRef World::findAny(const FBox &box, const FindFilter &filter) const {
		if((filter.m_flags & Flags::tile)) {
			int index = m_tile_map.findAny(box, filter.m_flags);
			if(index != -1)
				return ObjectRef(index, false);
		}

		if(filter.m_flags & Flags::entity) {
			int ignore_index = filterIgnoreIndex(filter);
			int index = m_entity_map.findAny(box, ignore_index, filter.m_flags);
			if(index != -1)
				return ObjectRef(index, true);
		}

		return ObjectRef();
	}

	void World::findAll(vector<ObjectRef> &out, const FBox &box, const FindFilter &filter) const {
		vector<int> inds;
		inds.reserve(1024);

		if(filter.m_flags & Flags::tile) {
			m_tile_map.findAll(inds, box, filter.m_flags);
			for(int n = 0; n < (int)inds.size(); n++)
				out.emplace_back(ObjectRef(inds[n], false));
			inds.clear();
		}

		if(filter.m_flags & Flags::entity) {
			int ignore_index = filterIgnoreIndex(filter);
			m_entity_map.findAll(inds, box, ignore_index, filter.m_flags);
			for(int n = 0; n < (int)inds.size(); n++)
				out.emplace_back(ObjectRef(inds[n], true));
		}
	}
		
	bool World::isVisible(const float3 &eye_pos, EntityRef target_ref, EntityRef ignore, int density) const {
		float step = 0.8f * (density == 1? 0.0f : 1.0f / float(density - 1));

		const Entity *target = const_cast<World*>(this)->refEntity(target_ref);
		if(!target)
			return false;
		const FBox &box = target->boundingBox();

		vector<float3> points = genPointsOnPlane(box, normalized(eye_pos - box.center()), density, false);
		vector<Segment> segments(points.size());
		for(int n = 0; n < (int)points.size(); n++)
			segments[n] = Segment(eye_pos, points[n]);

		vector<Intersection> isects;
		traceCoherent(segments, isects, {Flags::all | Flags::occluding, ignore});

		for(int n = 0; n < (int)points.size(); n++) {
			const Intersection &isect = isects[n];
			if(isect.isEmpty() || (ObjectRef)isect == ObjectRef(target_ref))
				return true;
		}

		return false;
	}

	bool World::isInside(const FBox &box) const {
		return m_tile_map.isInside(box);
	}
		
	const NaviMap *World::naviMap(int agent_size) const {
		for(int n = 0; n < (int)m_navi_maps.size(); n++)
			if(m_navi_maps[n].agentSize() == agent_size)
				return &m_navi_maps[n];

		return nullptr;
	}
		
	const NaviMap *World::accessNaviMap(const FBox &bbox) const {
		int agent_size = round(max(bbox.width(), bbox.depth()));
		return naviMap(agent_size);
	}
		
	bool World::findClosestPos(int3 &out, const int3 &source, const IBox &target_box, EntityRef agent_ref) const {
		const Entity *agent = const_cast<World*>(this)->refEntity(agent_ref);
		if(agent) {
			FBox bbox = agent->boundingBox();
			const NaviMap *navi_map = accessNaviMap(bbox);
			if(navi_map)
				return navi_map->findClosestPos(out, source, bbox.height(), target_box);
		}

		return false;
	}
		
	bool World::findPath(Path &out, const int3 &start, const int3 &end, EntityRef agent_ref) const {
		const Entity *agent = const_cast<World*>(this)->refEntity(agent_ref);
		if(agent) {
			const NaviMap *navi_map = accessNaviMap(agent->boundingBox());
			if(navi_map) {
				vector<int3> path;
				if(navi_map->findPath(path, start, end, agent_ref.index())) {
					out = Path(path);
					return true;
				}
			}
		}

		return false;
	}
	
	void World::replicate(const Entity *entity) {
		DASSERT(entity && entity->isHooked());
		DASSERT(entity->m_world == this);
		replicate(entity->index());
	}

	void World::replicate(int index) {
		if(isServer())
			m_replicator->replicateEntity(index);
	}

	void World::playSound(const char *name, const float3 &pos) {
		if(isServer())
			return;
		audio::playSound(name, pos);
	}

	void World::playSound(SoundId sound_id, const float3 &pos) {
		if(isServer())
			return;
		audio::playSound(sound_id, pos);
	}

	bool World::sendOrder(POrder &&order_ptr, EntityRef actor_ref) {
		DASSERT(order_ptr);

		if(isClient()) {
			m_replicator->replicateOrder(std::move(order_ptr), actor_ref);
			return true;
		}

		if( Actor *entity = refEntity<Actor>(actor_ref) )
			return entity->setOrder(std::move(order_ptr));
		return false;
	}

}
