// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/layer.h"

namespace hud {

class HudOptions : public HudLayer {
  public:
	HudOptions(const FRect &target_rect);
	~HudOptions() {}

  protected:
	bool onEvent(const HudEvent &) override;

	PHudButton m_exit_to_menu;
	PHudButton m_exit_to_system;
};

}
