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

		const vector<PlayableCharacter> playableCharacters() const override { return { m_pc }; };
		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;

	private:
		PlayableCharacter m_pc;
	};

}

#endif
