/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/game_mode.h"
#include "game/world.h"

namespace game {

	bool GameMode::isClient() const {
		return m_world.isClient();
	}

	bool GameMode::isServer() const {
		return m_world.isServer();
	}

}
