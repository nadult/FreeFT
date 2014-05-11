/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_STANCE_H
#define HUD_STANCE_H

#include "hud/base.h"
#include "hud/button.h"

namespace hud
{

	class HudStance: public HudButton {
	public:
		HudStance(const FRect &target_rect, Stance::Type stance, gfx::PTexture icons);

		void update(double time_diff) override;
		void draw() const override;

		Stance::Type stance() const { return m_stance_id; }

	private:
		FRect m_uv_rect;
		Stance::Type m_stance_id;
		gfx::PTexture m_icons;
	};

}

#endif
