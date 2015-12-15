/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_STATS_H
#define HUD_STATS_H

#include "hud/layer.h"

namespace hud
{

	class HudStats: public HudLayer {
	public:
		HudStats(const FRect &target_rect);
		~HudStats();

		bool canShow() const override;

	protected:
		void onUpdate(double time_diff) override;
		void onDraw(Renderer2D&) const override;
		void updateData();

		PHudGrid m_grid;
		string m_stats;
	};

}

#endif
