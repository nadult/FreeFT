/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor_ai.h"
#include "game/actor.h"

namespace game {

	ActorAI::ActorAI(PWorld world, EntityRef actor)
		:m_world(world), m_actor_ref(actor) {
	}
		
	Actor *ActorAI::actor() const {
		return m_world->refEntity<Actor>(m_actor_ref);
	}

	SimpleAI::SimpleAI(PWorld world, EntityRef ref) :ActorAI(world, ref) {
	}

	void SimpleAI::think() {
		Actor *actor = this->actor();
		if(!actor)
			return;

	}

}

