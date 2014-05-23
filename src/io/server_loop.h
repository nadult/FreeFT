/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_SERVER_LOOP_H
#define IO_SERVER_LOOP_H

#include "io/loop.h"
#include "io/controller.h"

namespace io {

	class ServerLoop: public Loop {
	public:
		ServerLoop(net::PServer &&server);
		bool tick(double time_diff) override;

	private:
		game::PWorld m_world;
		net::PServer m_server;
		PController m_controller;
	};

}

#endif
