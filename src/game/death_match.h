/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_DEATH_MATCH_H
#define GAME_DEATH_MATCH_H

#include "game/game_mode.h"

namespace game {

	class DeathMatch: public GameMode {
	public:
		DeathMatch(World &world);

		GameModeId::Type modeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
	};

}

#endif

