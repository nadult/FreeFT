/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_AI_H
#define GAME_ACTOR_AI_H

#include "game/world.h"

namespace game {

	class Actor;

	class ActorAI {
	public:
		ActorAI(PWorld world, EntityRef ref);
		virtual ~ActorAI() = default;

		virtual void think() = 0;
		virtual void onImpact(DeathTypeId::Type, float damage) { }
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
		ActorAI *clone() { return new SimpleAI(*this); }

		enum class Mode {
			idle,
			attacking,
		};

	protected:
		Mode m_mode;
		EntityRef m_target;
		float m_delay;
	};

}

#endif
