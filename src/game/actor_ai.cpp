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

	SimpleAI::SimpleAI(PWorld world, EntityRef ref) :ActorAI(world, ref), m_delay(0.0f) {
	}

	void SimpleAI::think() {
		Actor *actor = this->actor();
		if(!actor)
			return;

		m_delay -= m_world->timeDelta();
		if(m_delay > 0.0f)
			return;
		m_delay = 0.25f;

		if(actor->currentOrder() == OrderTypeId::idle) {
			vector<ObjectRef> close_ents;
			FBox close_prox = actor->boundingBox();
			close_prox.min -= float3(100, 0, 100);
			close_prox.max += float3(100, 0, 100);

			m_world->findAll(close_ents, close_prox, actor, Flags::actor);
			for(int n = 0; n < (int)close_ents.size(); n++) {
				Actor *nearby = m_world->refEntity<Actor>(close_ents[n]);
				if(nearby && nearby->factionId() != actor->factionId()) {
					m_target = nearby->ref();
				}
			}

			m_world->sendOrder(new TrackOrder(m_target, 1.0f, true), m_actor_ref);
		}

	}

}

