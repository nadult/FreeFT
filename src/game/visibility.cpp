/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/visibility.h"
#include "gfx/scene_renderer.h"
#include "game/tile.h"
#include <algorithm>
#include "sys/profiler.h"

namespace game {

	WorldViewer::WorldViewer(PWorld world, EntityRef spectator)
		:m_world(world), m_spectator(spectator), m_occluder_config(world->tileMap().occluderMap()) {
		DASSERT(m_world);
	}

	WorldViewer::~WorldViewer() {
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
	
	static const double blend_time = 0.5;
	static const float feel_distance = 50.0;
	static const float max_distance = 200.0;
		
	bool WorldViewer::isVisible(const FBox &box, int index, bool is_movable) const {
		float dist = distance(box.closestPoint(m_cur_pos), m_cur_pos);

		if(dist > max_distance)
			return false;

		if(dist > feel_distance && !isInsideFrustum(box, m_cur_pos, m_cur_dir, m_cur_fov))
			return false;

		if(!is_movable || m_spectator.index() == index)
			return true;

		enum { num_rays = 16 };
		const Entity *spectator = m_world->refEntity(m_spectator);

		for(int x = 0; x < 3; x++)
			for(int y = 0; y < 3; y++)
				for(int z = 0; z < 3; z++) {
					float3 target(
						box.min.x + box.width() * (0.1f + float(x) * 0.4f),
						box.min.y + box.height() * (0.1f + float(y) * 0.4f),
						box.min.z + box.depth() * (0.1f + float(z) * 0.4f) );
					float3 dir = target - m_eye_pos;
					float len = length(dir);
					Segment segment(Ray(m_eye_pos, dir / len), 0.0f, len);
					Intersection isect = m_world->trace(segment, spectator, collider_all);
					if(isect.isEntity() && ((ObjectRef)isect).index() == index)
						return true;
				}

		return false;
	}
		
	bool WorldViewer::isMovable(const Entity &entity) const {
		EntityId::Type type_id = entity.typeId();
		if(type_id == EntityId::actor)
			return !static_cast<const Actor&>(entity).isDead();
		return type_id == EntityId::projectile || type_id == EntityId::impact;
	}

	void WorldViewer::update(double time_diff) {
		PROFILE("WorldViewer::update");
		Entity *spectator = m_world->refEntity(m_spectator);

		if((int)m_entities.size() != m_world->entityCount())
			m_entities.resize(m_world->entityCount());
		
		if(!spectator) {
			for(int n = 0; n < (int)m_entities.size(); n++) {
				Entity *entity = m_world->refEntity(n);
				VisEntity &vis_entity = m_entities[n];
				vis_entity.ref = entity? entity->ref() : EntityRef();
				vis_entity.mode = entity? VisEntity::visible : VisEntity::invisible;
				vis_entity.occluder_id = -1;
			}
			return;
		}

		FBox bbox = spectator->boundingBox();
		m_cur_pos = bbox.center();
		m_eye_pos = asXZY(m_cur_pos.xz(), bbox.min.y + bbox.height() * 0.75f);
		m_cur_dir = asXZY(spectator->dir(), 0.0f);
		m_cur_fov = 0.1f;

		for(int n = 0; n < (int)m_entities.size(); n++) {
			Entity *entity = m_world->refEntity(n);
			VisEntity &vis_entity = m_entities[n];

			if(!entity) {
				vis_entity = VisEntity();
				continue;
			}
			
			const auto *desc = m_world->refDesc(EntityRef(n));
			DASSERT(desc);

			bool is_visible = isVisible(entity->boundingBox(), n, isMovable(*entity));

			if(vis_entity.ref != entity->ref()) {
				vis_entity = VisEntity();
				vis_entity.ref = entity->ref();
				vis_entity.mode = is_visible? VisEntity::visible : VisEntity::invisible;
			}

			if(is_visible) {
				vis_entity.occluder_id = desc->occluder_id;

				if(vis_entity.mode == VisEntity::visible)
					continue;

				if(vis_entity.mode == VisEntity::shadowed || vis_entity.mode == VisEntity::pre_blending_out) {
					vis_entity.shadow.reset();
					vis_entity.mode = VisEntity::visible;
				}
				else {
					float blend_value = 0.0;
					if(vis_entity.mode == VisEntity::blending_in)
						blend_value = vis_entity.blend_value;
					else if(vis_entity.mode == VisEntity::blending_out)
						blend_value = blend_time - vis_entity.blend_value;

					blend_value += time_diff;
					vis_entity.mode = blend_value > blend_time? VisEntity::visible : VisEntity::blending_in;
					vis_entity.blend_value = blend_value;
				}
			}
			else {
				if(vis_entity.mode == VisEntity::shadowed || vis_entity.mode == VisEntity::invisible)
					continue;

				if(vis_entity.mode == VisEntity::visible) {
					vis_entity.blend_value = 0.0f;
					vis_entity.mode = VisEntity::pre_blending_out;
				}

				if(vis_entity.mode == VisEntity::pre_blending_out) {
					vis_entity.blend_value += time_diff;

					if(vis_entity.blend_value > blend_time) {
						if(isMovable(*entity)) {
							vis_entity.mode = VisEntity::blending_out;
							vis_entity.blend_value = vis_entity.blend_value - blend_time;
						}
						else {
							vis_entity.mode = VisEntity::shadowed;
							vis_entity.occluder_id = desc->occluder_id;
							vis_entity.shadow.reset(entity->clone());
						}
					}
				}
				else {
					float blend_value = 0.0f;

					if(vis_entity.mode == VisEntity::blending_in)
						blend_value = blend_time - vis_entity.blend_value;
					else if(vis_entity.mode == VisEntity::blending_out)
						blend_value = vis_entity.blend_value;

					vis_entity.blend_value = blend_value + time_diff;
					vis_entity.mode = VisEntity::blending_out;

					if(vis_entity.blend_value > blend_time)
						vis_entity.mode = VisEntity::invisible;
				}
			}
		}

		if(m_occluder_config.update(spectator->boundingBox()))
			m_world->tileMap().updateVisibility(m_occluder_config);
	}
		
	const FBox WorldViewer::refBBox(ObjectRef ref) const {
		int index = ref.index();
		if(ref.isEntity() && index >= 0 && index < (int)m_entities.size()) {
			if(m_entities[index].mode == VisEntity::shadowed)
				return m_entities[index].shadow->boundingBox();
		}

		return m_world->refBBox(ref);
	}

	const Tile *WorldViewer::refTile(ObjectRef ref) const {
		return m_world->refTile(ref);
	}

	const Entity *WorldViewer::refEntity(EntityRef ref) const {
		int index = ref.index();
		if(index >= 0 && index < (int)m_entities.size()) {
			if(m_entities[index].mode == VisEntity::shadowed)
				return m_entities[index].shadow.get();
			if(m_entities[index].mode == VisEntity::invisible)
				return nullptr;
		}

		return m_world->refEntity(ref);
	}

	void WorldViewer::addToRender(gfx::SceneRenderer &renderer) {
		vector<int> inds;
		inds.reserve(8192);

		const TileMap &tile_map = m_world->tileMap();
		tile_map.findAll(inds, renderer.targetRect(), collider_all | visibility_flag);

		for(int n = 0; n < (int)inds.size(); n++)
			tile_map[inds[n]].ptr->addToRender(renderer, (int3)tile_map[inds[n]].bbox.min);

		for(int n = 0; n < (int)m_entities.size(); n++) {
			const VisEntity &vis_entity = m_entities[n];
			if(vis_entity.mode == VisEntity::invisible)
				continue;

			float blend_value = 1.0f;
			if(vis_entity.mode == VisEntity::blending_in)
				blend_value = vis_entity.blend_value / blend_time;
			else if(vis_entity.mode == VisEntity::blending_out)
				blend_value = 1.0f - vis_entity.blend_value / blend_time;

			const Entity *entity = vis_entity.mode == VisEntity::shadowed?
				vis_entity.shadow.get() : m_world->refEntity(vis_entity.ref);

			if(entity && m_occluder_config.isVisible(vis_entity.occluder_id))
				entity->addToRender(renderer, Color(1.0f, 1.0f, 1.0f, blend_value));
		}
	}
		
	Intersection WorldViewer::pixelIntersect(const int2 &screen_pos, int flags) const {
		Intersection out;
		FBox out_bbox;

		if(flags & collider_tiles) {
			const TileMap &tile_map = m_world->tileMap();
			vector<int> inds;
			tile_map.findAll(inds, IRect(screen_pos, screen_pos + int2(1, 1)), flags | visibility_flag);

			for(int i = 0; i < (int)inds.size(); i++) {
				const auto &desc = tile_map[inds[i]];
				
				FBox bbox = desc.bbox;
				
				if(out.isEmpty() || drawingOrder(bbox, out_bbox) == 1)
					if(desc.ptr->testPixel(screen_pos - worldToScreen((int3)bbox.min))) {
						out = ObjectRef(inds[i], false);
						out_bbox = bbox;
					}
			}
		}

		for(int n = 0; n < (int)m_entities.size(); n++) {
			const Entity *entity = refEntity(n);
			if(!entity || !m_occluder_config.isVisible(m_entities[n].occluder_id) || !(entity->colliderType() & flags))
				continue;
			FBox bbox = entity->boundingBox();

			if(out.isEmpty() || drawingOrder(bbox, out_bbox) == 1)
				if(entity->testPixel(screen_pos)) {
					out = ObjectRef(n, true);
					out_bbox = bbox;
				}
		}

		if(out.isEmpty())
			return Intersection();
		return Intersection(out, intersection(screenRay(screen_pos), out_bbox));
	}

	Intersection WorldViewer::trace(const Segment &segment, const Entity *ignore, int flags) const {
		Intersection out;

		if(flags & collider_tiles)
			out = m_world->trace(segment, nullptr, (flags & collider_tiles) | visibility_flag);

		if(flags & collider_entities)
			for(int n = 0; n < (int)m_entities.size(); n++) {
				const Entity *entity = refEntity(n);
				if(!entity || !m_occluder_config.isVisible(m_entities[n].occluder_id) || !(entity->colliderType() & flags))
					continue;

				float distance = intersection(segment, entity->boundingBox());
				if(distance < out.distance())
					out = Intersection(ObjectRef(n, true), distance);
			}

		return out;
	}

}
