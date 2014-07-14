/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PC_CONTROLLER_H
#define GAME_PC_CONTROLLER_H

#include "game/base.h"
#include "game/entity.h"

namespace game
{

	// Playable character controller
	class PCController: public RefCounter {
	public:
		PCController(World&, PPlayableCharacter);
		~PCController();

		PPlayableCharacter pc() const { return m_pc; }
		bool hasActor() const;

		bool canChangeStance() const;
		void setStance(Stance::Type);
		int targetStance() const { return m_target_stance; }
	
	private:
		Actor *getActor() const;
		void sendOrder(game::POrder&&);

		int m_target_stance;

		World &m_world;
		PPlayableCharacter m_pc;
		EntityRef m_actor_ref;
	};

}

#endif
