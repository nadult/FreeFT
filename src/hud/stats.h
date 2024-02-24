// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/layer.h"

namespace hud {

class HudStats : public HudLayer {
  public:
	HudStats(const FRect &target_rect);
	~HudStats();

	bool canShow() const override;

  protected:
	void onUpdate(double time_diff) override;
	void onDraw(Renderer2D &) const override;
	void updateData();

	PHudGrid m_grid;
	string m_stats;
};

}
