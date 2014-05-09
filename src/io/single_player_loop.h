/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_SINGLE_PLAYER_LOOP_H
#define IO_SINGLE_PLAYER_LOOP_H

#include "io/controller.h"
#include "io/loop.h"
#include "game/base.h"

namespace io {

	game::PWorld createWorld(const string &map_name);

	class SinglePlayerLoop: public Loop {
	public:
		SinglePlayerLoop(game::PWorld world);
		bool tick(double time_diff) override;

	private:
		game::PWorld m_world;
		PController m_controller;
		double m_time_multiplier;
	};

}

#endif
