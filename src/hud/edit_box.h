/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_EDIT_BOX_H
#define HUD_EDIT_BOX_H

#include "hud/widget.h"

namespace hud {

	class HudEditBox: public HudWidget {
	public:
		HudEditBox(const FRect &rect, int max_size);

		void update(const float2 &mouse_pos, double time_diff) override;
		void draw() const override;

	protected:
	};

}

#endif
