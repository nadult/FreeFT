/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_OPTIONS_H
#define HUD_OPTIONS_H

#include "hud/layer.h"

namespace hud
{

	class HudOptions: public HudLayer {
	public:
		HudOptions(const FRect &target_rect);
	};

}

#endif
