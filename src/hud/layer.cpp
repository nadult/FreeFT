// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/layer.h"

#include "game/actor.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"
#include "game/world.h"
#include "gfx/drawing.h"
#include <fwk/gfx/font.h>

namespace hud {

	HudLayer::HudLayer(const FRect &rect, SlideMode slide_mode)
		:HudWidget(rect), m_slide_mode(slide_mode) {
		m_anim_speed = 5.0f;
	}
		
	void HudLayer::setTitle(const string &title) {
		m_title = title;
		needsLayout();
	}

	HudLayer::~HudLayer() { }
		
	float HudLayer::topOffset() const {
		return m_title.empty()? 0.0f : m_big_font->lineHeight() + layer_spacing;
	}	
		
	const FRect HudLayer::rect() const {
		FRect out = m_rect;

		if(m_slide_mode == slide_left)
			out -= float2((1.0f - m_visible_time) * (m_rect.ex() + 5.0f), 0.0f);
		else if(m_slide_mode == slide_top)
			out -= float2(0.0f, (1.0f - m_visible_time) * (m_rect.ey() + 5.0f));

		return out;
	}
		
	float HudLayer::alpha() const {
		return 1.0f;
	}
		
	float HudLayer::backAlpha() const {
		return alpha() * 0.4f;
	}
		
	Color HudLayer::backColor() const {
		return (Color)mulAlpha(m_style.layer_color, backAlpha());
	}

	Color HudLayer::borderColor() const {
		return (Color)mulAlpha(m_style.layer_color, min(backAlpha(), 1.0f));
	}

	Color HudLayer::titleColor() const {
		float new_alpha = (0.25f + 0.75f * pow(m_visible_time, 4.0f)) * alpha();
		return (Color)mulAlpha(FColor(m_style.enabled_color), new_alpha);
	}
		
	Color HudLayer::titleShadowColor() const {
		return Color(ColorId::black, titleColor().a);
	}

	void HudLayer::onDraw(Renderer2D& out) const {
		FRect rect = this->rect();

		out.addFilledRect(rect, (FColor)backColor());
		drawBorder(out, rect, borderColor(), float2(0, 0), 100.0f);

		if(!m_title.empty()) {
			FRect font_rect = rect.inset(float2(spacing, layer_spacing));
			m_big_font->draw(out, font_rect, {titleColor(), titleShadowColor()}, m_title);
		}
	}

	void HudLayer::setWorld(game::PWorld world) {
		m_world = world;
	}
		
	void HudLayer::setPCController(game::PPCController controller) {
		m_pc_controller = std::move(controller);
		onPCControllerSet();
	}

}
