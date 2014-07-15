/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/layer.h"
#include "gfx/device.h"
#include "game/world.h"
#include "game/game_mode.h"
#include "game/actor.h"
#include "game/pc_controller.h"

using namespace gfx;

namespace hud {

	HudLayer::HudLayer(const FRect &rect, SlideMode slide_mode)
		:HudWidget(rect), m_slide_mode(slide_mode) {
		m_anim_speed = 5.0f;
	}

	HudLayer::~HudLayer() { }
		
	const FRect HudLayer::rect() const {
		FRect out = m_rect;

		if(m_slide_mode == slide_left)
			out -= float2((1.0f - m_visible_time) * (m_rect.max.x + 5.0f), 0.0f);
		else if(m_slide_mode == slide_top)
			out -= float2(0.0f, (1.0f - m_visible_time) * (m_rect.max.y + 5.0f));

		return out;
	}
		
	float HudLayer::backAlpha() const {
		return 0.3f;
	}

	void HudLayer::onDraw() const {
		Color color = m_style.layer_color;
		DTexture::unbind();
		drawQuad(rect(), mulAlpha(color, backAlpha()));
		drawBorder(rect(), mulAlpha(color, min(backAlpha() * 1.5f, 1.0f)), float2(0, 0), 100.0f);
	}
		
	void HudLayer::setPCController(game::PPCController controller) {
		m_pc_controller = std::move(controller);
	}

}
