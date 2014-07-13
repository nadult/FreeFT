/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/layer.h"
#include "gfx/device.h"
#include "game/world.h"
#include "game/game_mode.h"

using namespace gfx;

namespace hud {

	HudLayer::HudLayer(game::PWorld world, const FRect &rect)
		:HudWidget(rect), m_world(world), m_slide_left(true) {
		m_anim_speed = 5.0f;
	}

	HudLayer::~HudLayer() { }
		
	const FRect HudLayer::rect() const {
		return m_slide_left? 
			m_rect - float2((1.0f - m_visible_time) * (m_rect.max.x + 5.0f), 0.0f) :
			m_rect - float2(0.0f, (1.0f - m_visible_time) * (m_rect.max.y + 5.0f));
	}
		
	float HudLayer::backAlpha() const {
		return 0.3f;
	}

	void HudLayer::onDraw() const {
		Color color = m_style.layer_color;
		drawQuad(rect(), mulAlpha(color, backAlpha()));
		drawBorder(rect(), mulAlpha(color, min(backAlpha() * 1.5f, 1.0f)), float2(0, 0), 100.0f);
	}
		
	void HudLayer::setPC(game::PPlayableCharacter pc) {
		m_pc = std::move(pc);
		onPCSet();
	}
	
	void HudLayer::sendOrder(game::POrder &&order) {
		if(m_pc && m_world)
			m_world->sendOrder(std::move(order), m_pc->entityRef());
	}

}
