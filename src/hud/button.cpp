/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/button.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"

using namespace gfx;

namespace hud {

	HudButton::HudButton(const FRect &rect)
		:m_target_rect(rect), m_style(defaultStyle()), m_over_time(0.0f), m_focus_time(0.0f), m_accelerator(0) {
		setStyle(defaultStyle());
	}

	HudButton::~HudButton() { }
		
	void HudButton::setStyle(HudStyle style) {
		m_style = style;
		m_font = Font::mgr[style.font_name];
	}
		
	void HudButton::update(double time_diff) {
		float anim_speed = 20.0f;

		m_focus_time += (m_is_focused? 1.0f - m_focus_time : -m_focus_time) * time_diff * anim_speed;
		m_focus_time = clamp(m_focus_time, 0.0f, 1.0f);
		
		m_over_time += (isMouseOver()? 1.0f - m_over_time : -m_over_time) * time_diff * anim_speed;
		m_over_time = clamp(m_over_time, 0.0f, 1.0f);
		m_over_time = max(m_over_time, m_focus_time);
	}

	Color HudButton::focusColor() const {
		return lerp(Color(m_style.focus_color, 160), m_style.focus_color, m_focus_time);
	}

	void HudButton::draw() const {
		DTexture::bind0();
		FRect rect = this->rect();

		drawQuad(rect, Color(m_style.back_color, 127));

		Color border_color = lerp(Color(m_style.border_color, 127), m_style.border_color, m_focus_time);
		float offset = lerp(m_style.border_offset, 0.0f, m_over_time);

		drawBorder(rect, border_color, float2(offset, offset), 20.0f, true);
		drawBorder(rect, border_color, float2(offset, offset), 20.0f, false);

		if(!m_text.empty()) {
			IRect extents = m_font->evalExtents(m_text.c_str());
			float2 pos = rect.center() - float2(extents.size()) * 0.5f - float2(0, 3);
			m_font->drawShadowed((int2)pos, focusColor(), Color::black, "%s", m_text.c_str());
		}
	}
		
	bool HudButton::testAccelerator() const {
		return m_accelerator && isKeyDown(m_accelerator);
	}

	bool HudButton::isMouseOver() const {
		return rect().isInside((float2)getMousePos());
	}
		
	const FRect HudButton::rect() const {
		return m_target_rect;
	}
		
	void HudButton::setText(const string &text) {
		m_text = text;
	}

}
