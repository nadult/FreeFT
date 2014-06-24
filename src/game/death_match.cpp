/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/death_match.h"
#include "game/world.h"

namespace game {

	DeathMatch::DeathMatch(World &world) :GameMode(world) {
		DASSERT(isClient() || isServer());
	}

	void DeathMatch::tick(double time_diff) {
	}

}
