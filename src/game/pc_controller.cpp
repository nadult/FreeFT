/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/pc_controller.h"
#include "game/character.h"
#include "game/actor.h"


namespace game {

	PCController::PCController(World &world, PPlayableCharacter pc)
		:m_world(world), m_pc(pc) {
		if(m_pc)
			m_actor_ref = m_pc->entityRef();

		Actor *actor = getActor();
		m_target_stance = actor? actor->stance() : -1;
	}
		
	PCController::~PCController() { }
		
	Actor *PCController::getActor() const {
		return m_world.refEntity<Actor>(m_actor_ref);
	}
	
	void PCController::sendOrder(game::POrder &&order) {
		m_world.sendOrder(std::move(order), m_actor_ref);
	}
		
	bool PCController::hasActor() const {
		return getActor() != nullptr;
	}

	bool PCController::canChangeStance() const {
		return hasActor();
	}

	void PCController::setStance(Stance::Type stance) {
		DASSERT(canChangeStance());
		sendOrder(new ChangeStanceOrder(stance));
		m_target_stance = stance;
	}

}
