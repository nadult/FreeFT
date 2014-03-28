/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/visibility.h"
#include "gfx/scene_renderer.h"
#include <algorithm>

namespace game {

	EntityShadow::EntityShadow(const Entity &rhs, int occluder_id, EntityRef true_ref)
		:Entity(rhs), m_occluder_id(occluder_id), m_true_ref(true_ref) {
	}

	WorldVisInfo::WorldVisInfo(PWorld world, EntityRef source)
		:m_world(world), m_source(source), m_initial_update(true) {
		DASSERT(m_world);
		m_world->addListener(this);
	}

	WorldVisInfo::~WorldVisInfo() {
		m_world->removeListener(this);
	}

	static bool isInsideFrustum(const FBox &box, const float3 &pos, const float3 &dir, float min_dot) {
		float3 corners[8];
		box.getCorners(corners);

		for(int n = 0; n < COUNTOF(corners); n++) {
			float3 cvector = corners[n] - pos;
			float len = length(cvector);

			if(dot(cvector, dir) >= min_dot * len)
				return true;
		}

		return false;
	}
		
	bool WorldVisInfo::isVisible(const FBox &box) const {
		if(isInsideFrustum(box, m_cur_pos, m_cur_dir, m_cur_fov)) {
			return true;
		}

		return false;
	}
		
	bool WorldVisInfo::isMovable(const Entity &entity) const {
		EntityId::Type type_id = entity.typeId();
		if(type_id == EntityId::actor)
			return !static_cast<const Actor&>(entity).isDead();
		return type_id == EntityId::projectile || type_id == EntityId::impact;
	}

	static const double blend_time = 0.5;

	void WorldVisInfo::onAddEntity(EntityRef ref) {
		const FBox &entity_bbox = m_world->refBBox(ref);
		if(isVisible(entity_bbox))
			m_infos.push_back(EntityVisInfo{ref, blend_time});
	}

	void WorldVisInfo::onSimulate(double time_diff) {
		Entity *source_entity = m_world->refEntity(m_source);
		//DASSERT(source_entity);
		if(!source_entity)
			return;

		m_cur_pos = source_entity->pos();
		m_cur_dir = asXZY(source_entity->dir(), 0.0f);
		m_cur_fov = 0.1f;

		//TODO: remove visibility_flag from grid?
		
		vector<EntityVisInfo> visible;
		double initial_time = m_initial_update? blend_time : 0.0;
		m_initial_update = false;

		for(int n = 0; n < m_world->entityCount(); n++) {
			Entity *entity = m_world->refEntity(n);

			if(entity && (entity->colliderType() & (collider_entities & ~collider_static)))
				if(isVisible(entity->boundingBox()))
					visible.emplace_back(EntityVisInfo{entity->ref(), initial_time});
		}

		std::sort(visible.begin(), visible.end());
		std::sort(m_infos.begin(), m_infos.end());

		vector<EntityVisInfo> new_vec;
		new_vec.reserve(m_infos.size());

		int i = 0, v = 0;
		while(i < (int)m_infos.size() && v < (int)visible.size()) {
			EntityVisInfo &iinfo = m_infos[i];
			EntityVisInfo &vinfo = visible[v];

			if(iinfo.ref == vinfo.ref) {
				double vis_time = iinfo.vis_time;
				if(vis_time < 0.0)
					vis_time = vis_time < -blend_time? blend_time * 2.0 + vis_time : blend_time;
				vis_time += time_diff;
				new_vec.emplace_back(EntityVisInfo{iinfo.ref, vis_time});
				i++; v++;
			}
			else {
				if(iinfo.ref < vinfo.ref) {
					double vis_time = min(iinfo.vis_time, (double)-constant::epsilon) - time_diff;
					if(vis_time >= -blend_time && canAddShadow(iinfo.ref))
						addShadow(iinfo.ref);
					else if(vis_time > -blend_time * 2)
						new_vec.emplace_back(EntityVisInfo{iinfo.ref, vis_time});
					i++;
				}
				else {
					new_vec.emplace_back(vinfo);
					v++;
				}
			}
		}
		while(i < (int)m_infos.size()) {
			EntityVisInfo &iinfo = m_infos[i];
			double vis_time = min(iinfo.vis_time, (double)-constant::epsilon) - time_diff;
			if(vis_time >= -blend_time && canAddShadow(iinfo.ref))
				addShadow(iinfo.ref);
			else if(vis_time > -blend_time * 2)
				new_vec.emplace_back(EntityVisInfo{iinfo.ref, vis_time});
			i++;
		}
		if(v < (int)visible.size())
			new_vec.insert(new_vec.end(), visible.begin() + v, visible.end());
		m_infos.swap(new_vec);

		for(int n = 0; n < (int)m_infos.size(); n++) {
			auto it = m_memory.find(m_infos[n].ref.index());
			if(it != m_memory.end()) {
				m_memory.erase(it);
				m_infos[n].vis_time = blend_time;
			}
		}

		for(auto it = m_memory.begin(); it != m_memory.end();) {
			if(!m_world->refEntity(it->second.trueRef()))
				m_memory.erase(it++);
			else
				++it;
		}
	}

	bool WorldVisInfo::canAddShadow(EntityRef ref) {
		const Entity *entity = m_world->refEntity(ref);
		return entity && !isMovable(*entity);
	}

	void WorldVisInfo::addShadow(EntityRef ref) {
		const Entity *entity = m_world->refEntity(ref);
		const Grid::ObjectDef *desc = m_world->refDesc((ObjectRef)ref);

		if(entity && desc && canAddShadow(ref)) {
			auto it = m_memory.find(ref.index());
			if(it != m_memory.end())
				m_memory.erase(it);
			m_memory.emplace(ref.index(), EntityShadow(*entity, desc->occluder_id, ref));
		}
	}
		
	void WorldVisInfo::addToRender(gfx::SceneRenderer &renderer) {
		for(int n = 0; n < (int)m_infos.size(); n++) {
			const Grid::ObjectDef *def = m_world->refDesc((ObjectRef)m_infos[n].ref);
			int flags = def->flags;
			if(!(flags & visibility_flag))
				continue;
			const Entity *entity = m_world->refEntity(m_infos[n].ref);
			double blend = m_infos[n].vis_time / blend_time;
			blend = clamp(blend < 0? 2.0 + blend : blend, 0.0, 1.0);

			if(entity)
				entity->addToRender(renderer, Color(1.0f, 1.0f, 1.0f, blend));
		}

		const OccluderMap &occmap = m_world->tileMap().occluderMap();

		for(auto it = m_memory.begin(); it != m_memory.end(); it++) {
			EntityShadow &shadow = it->second;

			if(shadow.occluderId() == -1 || occmap[shadow.occluderId()].is_visible) {
				shadow.addToRender(renderer);
			}
		}
	}

}
