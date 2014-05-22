/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MULTI_PLAYER_LOOP_H
#define IO_MULTI_PLAYER_LOOP_H

#include "io/loop.h"
#include "io/controller.h"
#include "hud/layer.h"
#include "hud/widget.h"
#include "net/lobby.h"

namespace hud {

	class MultiPlayerMenu: public HudLayer {
	public:
		MultiPlayerMenu(const FRect &rect, HudStyle style);

		void update(bool is_active, double time_diff) override;
		void draw() const override;

	protected:
		vector<PHudWidget> m_buttons;
		vector<net::ServerStatusChunk> m_servers;
	};

}

namespace io {

	net::PClient createClient(const string &server_name, int port);

	class MultiPlayerLoop: public Loop {
	public:
		MultiPlayerLoop(net::PClient client);
		bool tick(double time_diff) override;

	private:
		net::PClient m_client;
		game::PWorld m_world;
		PController m_controller;
	};

}

#endif
