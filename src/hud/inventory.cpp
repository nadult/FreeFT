/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/inventory.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"

using namespace gfx;

namespace hud {

	HudInventory::HudInventory(const FRect &target_rect)
		:HudButton(target_rect) {
	}
		
}
