/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MULTI_PLAYER_LOOP_H
#define IO_MULTI_PLAYER_LOOP_H

#include "io/loop.h"
#include "io/controller.h"

namespace io {

	class MultiPlayerLoop: public Loop {
	public:
		MultiPlayerLoop(const string &server_address, int port);

		bool tick(double time_diff) override;

	private:
		net::PClient m_client;
		game::PWorld m_world;
		PController m_controller;
	};

}

#endif
