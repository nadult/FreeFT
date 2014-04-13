/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_AI_H
#define GAME_ACTOR_AI_H

#include "game/world.h"
#include "game/orders.h"

namespace game {

	class Actor;

	class ActorAI {
	public:
		ActorAI(PWorld world, EntityRef ref);
		virtual ~ActorAI() = default;

		virtual void think() = 0;
		virtual void onImpact(DamageType::Type, float damage, const float3 &force) { }
		virtual void onFailed(OrderTypeId::Type) { }
		virtual ActorAI *clone() = 0;

		Actor *actor() const;

	protected:
		PWorld m_world;
		EntityRef m_actor_ref;
	};

	typedef ClonablePtr<ActorAI> PActorAI;

	class SimpleAI: public ActorAI {
	protected:
		SimpleAI(PWorld world, EntityRef actor);
		friend class Actor;

	public:
		void think() override;
		void tryEquipItems();

		ActorAI *clone() { return new SimpleAI(*this); }
		void onImpact(DamageType::Type, float damage, const float3 &force) override;
		void onFailed(OrderTypeId::Type) override;
		void findEnemies(const float3 &range, vector<EntityRef> &out);

		enum class Mode {
			idle,
			attacking_melee,
			attacking_ranged,
		};

	protected:
		Mode m_mode;
		EntityRef m_target;
		float m_last_time_visible;
		float m_delay;
		int m_failed_orders;
	};

}

#endif
