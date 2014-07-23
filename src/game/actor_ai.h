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
	class ActorAI {
	public:
		ActorAI(PWorld world, EntityRef ref);
		virtual ~ActorAI() = default;

		virtual void think() = 0;
		virtual void onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) { }
		virtual void onFailed(OrderTypeId::Type) { }
		virtual ActorAI *clone() = 0;
		virtual const string status() const { return string(); }

		Actor *actor() const;
		int factionId() const;

	protected:
		PWorld m_world;
		EntityRef m_actor_ref;
	};

	typedef ClonablePtr<ActorAI> PActorAI;

	template <class TAI, class ...Args>
	void ThinkingEntity::attachAI(PWorld world, const Args&... args) {
		m_ai = new TAI(world, ref(), args...);
	}

	class SimpleAI: public ActorAI {
	protected:
		SimpleAI(PWorld world, EntityRef actor);
		friend class ThinkingEntity;

	public:
		void think() override;
		const Weapon findBestWeapon() const;

		ActorAI *clone() { return new SimpleAI(*this); }
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
