/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/death_match.h"
#include "game/world.h"

namespace game {

	DeathMatchServer::DeathMatchServer(World &world) :GameModeServer(world) {
	}

	void DeathMatchServer::tick(double time_diff) {
	}


	DeathMatchClient::DeathMatchClient(World &world, int client_id) :GameModeClient(world, client_id) {
	}

	void DeathMatchClient::tick(double time_diff) {
	}


}
