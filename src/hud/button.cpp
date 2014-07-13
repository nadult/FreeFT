/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/button.h"
#include "gfx/device.h"
#include "gfx/font.h"

using namespace gfx;

namespace hud {

	namespace {

		struct IconInfo {
			FRect uv_rect;
		};

		IconInfo s_icons[HudIcon::count] = {
			{ FRect(0.00f, 0.00f, 0.25f, 0.25f) },
			{ FRect(0.25f, 0.00f, 0.50f, 0.25f) },
			{ FRect(0.50f, 0.00f, 0.75f, 0.25f) },

			{ FRect(0.00f, 0.25f, 0.25f, 0.50f) },
			{ FRect(0.25f, 0.25f, 0.50f, 0.50f) },
			{ FRect(0.50f, 0.25f, 0.75f, 0.50f) },
			{ FRect(0.75f, 0.25f, 1.00f, 0.50f) },

			{ FRect(0.00f, 0.50f, 0.25f, 0.75f) }
		};

	}

	HudButton::HudButton(const FRect &rect, HudEvent::Type event_type, int event_value)
		:HudWidget(rect), m_highlighted_time(0.0f), m_enabled_time(0.0f), m_is_enabled(false), m_is_highlighted(false),
		 m_accelerator(0), m_icon_id(HudIcon::undefined), m_event_type(event_type), m_event_value(event_value) {
		m_icons_tex = gfx::DTexture::mgr["icons.png"];
	}

	HudButton::~HudButton() { }
		
	void HudButton::onUpdate(double time_diff) {
		animateValue(m_enabled_time, time_diff * m_anim_speed, m_is_enabled);
		animateValue(m_highlighted_time, time_diff * m_anim_speed, m_is_highlighted);
		m_highlighted_time = max(m_highlighted_time, m_enabled_time);
	}

	void HudButton::onDraw() const {
		FRect rect = this->rect();
		drawQuad(rect, backgroundColor());

		u8 border_alpha = clamp((int)(255 * this->alpha() * (0.3f + 0.7f * m_enabled_time * m_highlighted_time)), 0, 255);
		Color border_color(m_style.border_color, border_alpha);
		float offset = lerp(m_style.border_offset, 0.0f, m_highlighted_time);
		drawBorder(rect, border_color, float2(offset, offset), 20.0f);

		if(!m_text.empty())
			m_font->draw(rect, {enabledColor(), enabledShadowColor(), HAlign::center, VAlign::center}, m_text);

		if(HudIcon::isValid(m_icon_id)) {
			m_icons_tex->bind();
			drawQuad(rect, s_icons[m_icon_id].uv_rect, enabledColor());
		}
	}
		
	bool HudButton::onInput(const io::InputEvent &event) {
		if(event.mouseMoved()) {
			m_is_highlighted = isMouseOver(event);
		}
		if( (event.mouseKeyDown(0) && isMouseOver(event)) || (m_accelerator && event.keyDown(m_accelerator)) ) {
			handleEvent(HudEvent(this, m_event_type, m_event_value));
			return true;
		}

		return false;
	}

	Color HudButton::enabledColor() const {
		Color out = lerp(Color(m_style.enabled_color, 160), m_style.enabled_color, m_enabled_time);
		return Color(out, u8(float(out.a) * alpha()));
	}
		
	Color HudButton::enabledShadowColor() const {
		return mulAlpha(Color::black, alpha());
	}

	Color HudButton::backgroundColor() const {
		return Color(m_style.back_color, (int)(alpha() * 127));
	}

	void HudButton::setEnabled(bool is_enabled, bool animate) {
		m_is_enabled = is_enabled;
		if(!animate)
			m_enabled_time = m_is_enabled? 1.0f : 0.0f;
	}
	
}
