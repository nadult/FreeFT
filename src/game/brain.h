/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_AI_H
#define GAME_ACTOR_AI_H

#include "game/world.h"
#include "game/orders.h"

namespace game {

	class Actor;

	//TODO: better name
	class Brain {
	public:
		Brain(World *world, EntityRef ref);
		virtual ~Brain() = default;

		virtual void think() = 0;
		virtual void onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) { }
		virtual void onFailed(OrderTypeId::Type) { }
		virtual Brain *clone() const = 0;
		virtual const string status() const { return string(); }

		ThinkingEntity *entity() const;
		Actor *actor() const;
		int factionId() const;

	protected:
		World *m_world;
		EntityRef m_entity_ref;
	};

	using PBrain = ::ClonablePtr<Brain>;

	template <class TAI, class ...Args>
	void ThinkingEntity::attachAI(World *world, const Args&... args) {
		m_ai = new TAI(world, ref(), args...);
	}

	class ActorBrain: public Brain {
	protected:
		ActorBrain(World *world, EntityRef entity);
		friend class ThinkingEntity;

	public:
		void think() override;
		const Weapon findBestWeapon() const;

		Brain *clone() const override { return new ActorBrain(*this); }
		void onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) override;
		void onFailed(OrderTypeId::Type) override;
		void findActors(int faction_id, const float3 &range, vector<EntityRef> &out);
		void informBuddies(EntityRef enemy);
		
		const string status() const override;

		const float3 findClosePos(float range) const;
		void tryRandomMove();

		enum class Mode {
			idle,
			attacking_melee,
			attacking_ranged,
		};

		void setEnemyFactions(const vector<int>&);
		bool isEnemy(int faction_id) const;

	protected:
		Mode m_mode;
		EntityRef m_target;
		float m_last_time_visible;
		float m_delay;
		float m_move_delay;
		int m_failed_orders, m_faction_id;
		vector<int> m_enemy_factions;
		mutable string m_last_message;
	};

}

#endif
