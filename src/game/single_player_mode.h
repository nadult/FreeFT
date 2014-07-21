/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_SINGLE_PLAYER_MODE_H
#define GAME_SINGLE_PLAYER_MODE_H

#include "game/game_mode.h"

namespace game {

	class SinglePlayerMode: public GameMode {
	public:
		SinglePlayerMode(World &world, Character character);

		GameModeId::Type typeId() const override { return GameModeId::single_player; }
		void tick(double time_diff) override;

	private:
		PlayableCharacter *m_pc;
	};

}

#endif
