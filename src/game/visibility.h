/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_VISIBILITY_H
#define GAME_VISIBILITY_H

#include "game/world.h"
#include "game/actor.h"
#include "game/entity_map.h"

namespace gfx { class SceneRenderer; }

namespace game {

	// Shadow is used to render dynamic entities that were once visible
	class EntityShadow: public Entity {
	public:
		EntityShadow(const Entity&, int occluder_id, EntityRef true_ref);
		EntityShadow(const EntityShadow&) = default;

		Entity *clone() const { return new EntityShadow(*this); }
		EntityId::Type typeId() const { return EntityId::shadow; }
		ColliderFlags colliderType() const { return collider_none; }

		int occluderId() const { return m_occluder_id; }
		EntityRef trueRef() const { return m_true_ref; }

	private:
		int m_occluder_id;
		EntityRef m_true_ref;
	};

	class WorldVisInfo: public WorldListener {
	public:
		WorldVisInfo(PWorld, EntityRef source);
		~WorldVisInfo();
		WorldVisInfo(const WorldVisInfo&) = delete;
		void operator=(const WorldVisInfo&) = delete;

		struct EntityVisInfo {
			EntityRef ref;
			double vis_time; // >= 0: time visible, < 0: time invisible

			bool operator<(const EntityVisInfo &rhs) const { return ref < rhs.ref; }
		};

		void addToRender(gfx::SceneRenderer&);

	protected:
		void onSimulate(double time_diff) override;
		void onAddEntity(EntityRef) override;

		bool isMovable(const Entity&) const;
		bool isVisible(const FBox &box) const;
		bool canAddShadow(EntityRef);
		void addShadow(EntityRef);

		vector<EntityVisInfo> m_infos;
		std::map<int, EntityShadow> m_memory;
		PWorld m_world;

		EntityRef m_source;
		float3 m_cur_pos;
		float3 m_cur_dir;
		float m_cur_fov;

		bool m_initial_update;
	};

}

#endif
