/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/edit_box.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"

using namespace gfx;

namespace hud {

	HudEditBox::HudEditBox(const FRect &rect, int max_size)
		:HudButton(rect) {

	}

	void HudEditBox::onUpdate(double time_diff) {
	}

	void HudEditBox::onDraw() const {
	}

}
