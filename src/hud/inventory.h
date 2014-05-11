/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_INVENTORY_H
#define HUD_INVENTORY_H

#include "game/base.h"
#include "game/entity.h"
#include "game/weapon.h"
#include "hud/button.h"

namespace hud
{

	class HudInventory: public HudButton {
	public:
		HudInventory(const FRect &target_rect);
	};

}

#endif
