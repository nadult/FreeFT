/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gameui/hud.h"
#include "gfx/device.h"

namespace ui {

	HUD::HUD() :Window(IRect(0, 0, 10, 10)) {
		m_back = gfx::DTexture::gui_mgr["back/game_bar_long"];
		int2 res = gfx::getWindowSize();
		IRect rect(res.x / 2 - m_back->width() / 2, res.y - m_back->height(), res.x / 2 + m_back->width() / 2, res.y);
		setRect(rect);

	}

	void HUD::drawContents() const {
		using namespace gfx;

		m_back->bind();
		drawQuad(int2(0, 0), m_back->dimensions());
	}

}
