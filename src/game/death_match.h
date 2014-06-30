/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_DEATH_MATCH_H
#define GAME_DEATH_MATCH_H

#include "game/game_mode.h"

namespace game {

	class DeathMatchServer: public GameModeServer {
	public:
		DeathMatchServer(World &world);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
	};

	class DeathMatchClient: public GameModeClient {
	public:
		DeathMatchClient(World &world, int client_id);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
	};


}

#endif

