/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/widget.h"
#include "game/actor.h"
#include "game/world.h"
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
			{ FRect(0.50f, 0.25f, 0.75f, 0.50f) }
		};

	}

	HudWidget::HudWidget(const FRect &rect)
		:m_target_rect(rect), m_style(defaultStyle()), m_over_time(0.0f), m_focus_time(0.0f), m_visible_time(1.0f),
		 m_is_focused(false), m_is_visible(true), m_accelerator(0), m_icon_id(HudIcon::undefined) {
		setStyle(defaultStyle());
		m_icons_tex = gfx::DTexture::mgr["icons.png"];
	}

	HudWidget::~HudWidget() { }
		
	void HudWidget::setStyle(HudStyle style) {
		m_style = style;
		m_font = Font::mgr[style.font_name];
		m_big_font = Font::mgr[style.big_font_name];
	}
		
	void HudWidget::update(const float2 &mouse_pos, double time_diff) {
		float anim_speed = 10.0f;
		animateValue(m_focus_time, time_diff * anim_speed, m_is_focused);
		animateValue(m_visible_time, time_diff * anim_speed, m_is_visible);
		animateValue(m_over_time, time_diff * anim_speed, isMouseOver(mouse_pos));
		m_over_time = max(m_over_time, m_focus_time);
	}

	Color HudWidget::focusColor() const {
		Color out = lerp(Color(m_style.focus_color, 160), m_style.focus_color, m_focus_time);
		return Color(out, u8(float(out.a) * alpha()));
	}
		
	Color HudWidget::focusShadowColor() const {
		return mulAlpha(Color::black, alpha());
	}

	Color HudWidget::backgroundColor() const {
		return Color(m_style.back_color, (int)(alpha() * 127));
	}

	void HudWidget::draw() const {
		if(!isVisible())
			return;

		DTexture::unbind();
		FRect rect = this->rect();

		u8 alpha(this->alpha() * 255);
		drawQuad(rect, backgroundColor());

		Color border_color = lerp(Color(m_style.border_color, alpha / 2), Color(m_style.border_color, alpha), m_focus_time);
		float offset = lerp(m_style.border_offset, 0.0f, m_over_time);
		drawBorder(rect, border_color, float2(offset, offset), 20.0f);

		if(!m_text.empty())
			m_font->draw(rect, {focusColor(), focusShadowColor(), HAlign::center, VAlign::center}, m_text);

		if(HudIcon::isValid(m_icon_id)) {
			m_icons_tex->bind();
			drawQuad(rect, s_icons[m_icon_id].uv_rect, focusColor());
		}
	}
		
	void HudWidget::setVisible(bool is_visible, bool animate) {
		m_is_visible = is_visible;
		if(!animate)
			m_visible_time = m_is_visible? 1.0f : 0.0f;
	}
	
	bool HudWidget::isVisible() const {	
		return m_is_visible || m_visible_time > 0.01f;
	}
		
	bool HudWidget::isMouseOver(const float2 &mouse_pos) const {
		return m_is_visible && rect().isInside(mouse_pos);
	}
		
	bool HudWidget::isPressed(const float2 &mouse_pos, int mouse_key, bool *is_accelerator) const {
		if(!m_is_visible)
			return false;
		if(m_accelerator && isKeyDown(m_accelerator)) {
			if(is_accelerator)
				*is_accelerator = true;
			return true;
		}
		return isMouseOver(mouse_pos) && isMouseKeyDown(mouse_key);
	}
	
	const FRect HudWidget::rect() const {
		return m_target_rect;
	}
		
	void HudWidget::setText(const string &text) {
		m_text = text;
	}

}
