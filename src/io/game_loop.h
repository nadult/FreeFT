// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "io/loop.h"
#include "sys/config.h"

namespace io {

class GameLoop : public Loop {
  public:
	GameLoop(GfxDevice *, net::PServer &&, bool from_main_menu);
	GameLoop(GfxDevice &, net::PClient &&, bool from_main_menu);
	GameLoop(GfxDevice &, game::PWorld, bool from_main_menu);
	~GameLoop();

  protected:
	GameLoop(const char *config_name, bool from_main_menu);

	bool onTick(double) override;
	void onDraw(Canvas2D &) override;
	void onTransitionFinished() override;

	enum Mode {
		mode_normal,
		mode_exiting_to_main,
		mode_exiting_to_system,
		mode_transitioning,
	} m_mode,
		m_next_mode;

	bool m_from_main_menu;
	Config m_config;

	net::PClient m_client;
	net::PServer m_server;
	game::PWorld m_world;
	io::PController m_controller;
};

}
