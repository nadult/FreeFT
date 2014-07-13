/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_EDIT_BOX_H
#define HUD_EDIT_BOX_H

#include "hud/button.h"

namespace hud {

	class HudEditBox: public HudButton {
	public:
		HudEditBox(const FRect &rect, int max_size);

		void onUpdate(double time_diff) override;
		void onDraw() const override;

	protected:
	};

}

#endif
