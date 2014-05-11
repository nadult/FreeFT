/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/stance.h"

using namespace gfx;


namespace hud {

	namespace {

		struct StanceInfo {
			Stance::Type stance_id;
			int accelerator;
			FRect uv_rect;
		};

		StanceInfo s_stances[Stance::count] = {
			{ Stance::stand,	'Q', FRect(0.00f, 0.00f, 0.25f, 0.25f) },
			{ Stance::crouch,	'A', FRect(0.25f, 0.00f, 0.50f, 0.25f) },
			{ Stance::prone,	'Z', FRect(0.50f, 0.00f, 0.75f, 0.25f) },
		};

	}

	HudStance::HudStance(const FRect &target_rect, Stance::Type stance, PTexture icons)
		:HudWidget(target_rect), m_stance_id(stance), m_icons(icons) {
		DASSERT(Stance::isValid(stance));
		DASSERT(m_icons);

		for(int n = 0; n < Stance::count; n++)
			if(s_stances[n].stance_id == m_stance_id) {
				m_uv_rect = s_stances[n].uv_rect;
				setAccelerator(s_stances[n].accelerator);
			}
	}
		
	void HudStance::draw() const {
		HudWidget::draw();
		m_icons->bind();
		drawQuad(rect(), m_uv_rect, focusColor());
	}

}
