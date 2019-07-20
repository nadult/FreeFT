// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_VISIBILITY_H
#define GAME_VISIBILITY_H

#include "game/world.h"
#include "game/actor.h"
#include "game/entity_map.h"
#include "occluder_map.h"

namespace gfx { class SceneRenderer; }

namespace game {

	class WorldViewer {
	public:
		WorldViewer(PWorld, EntityRef spectator = EntityRef());
		~WorldViewer();
		WorldViewer(const WorldViewer&) = delete;
		void operator=(const WorldViewer&) = delete;

		void setSpectator(EntityRef);

		struct VisEntity {
			VisEntity() :mode(invisible), occluder_id(-1) { }

			unique_ptr<Entity> shadow;
			EntityRef ref;

			enum Mode {
				blending_out,
				blending_in,
				pre_blending_out,
				visible,
				invisible,
				shadowed,
			} mode;

			float blend_value;
			int occluder_id;
		};

		// WorldViewer has to be updated after each world simulation
		void update(double time_diff);
		void addToRender(SceneRenderer&) const;

		const FBox refBBox(ObjectRef) const;
		const Tile *refTile(ObjectRef) const;
		const Entity *refEntity(EntityRef) const;
		const Entity *refEntity(int index) const; //TODO: meh

		//TODO: make it impossible to reference shadows? (only getting their type & bbox)

		Intersection pixelIntersect(const int2 &screen_pos, const FindFilter &filter = FindFilter()) const;
		Intersection trace(const Segment3F &segment, const FindFilter &filter = FindFilter()) const;

		void setSeeAll(bool see_all) { m_see_all = see_all; }

	protected:		
		bool isMovable(const Entity &entity) const;

		vector<VisEntity> m_entities;
		OccluderConfig m_occluder_config;

		PWorld m_world;

		EntityRef m_spectator;
		float3 m_cur_pos, m_eye_pos;
		bool m_see_all;
	};

}

#endif
