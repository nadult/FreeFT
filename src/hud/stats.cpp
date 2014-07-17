/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/stats.h"
#include "game/pc_controller.h"
#include "game/world.h"

using namespace gfx;

namespace hud {

	HudStats::HudStats(const FRect &target_rect)
		:HudLayer(target_rect) {
		setTitle("Statistics:");
	}
		
	HudStats::~HudStats() { }
	
	bool HudStats::canShow() const {
		return m_pc_controller && m_pc_controller->world().isClient();
	}
		
}
