/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/button.h"
#include "gfx/drawing.h"

using namespace gfx;

namespace hud {

	namespace {

		struct IconInfo {
			FRect uv_rect;
		};

		static const EnumMap<HudIcon, IconInfo> s_icons = {
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

	HudButton::HudButton(const FRect &rect, int id)
		:HudWidget(rect), m_button_style(HudButtonStyle::normal), m_label_style(HudLabelStyle::center), m_id(id) {
		m_icons_tex = res::textures()["icons.png"];
	}

	HudButton::~HudButton() { }
		
	void HudButton::onUpdate(double time_diff) {
		animateValue(m_enabled_time, time_diff * m_anim_speed, m_is_enabled);
		animateValue(m_greyed_time, time_diff * m_anim_speed, m_is_greyed);
		animateValue(m_highlighted_time, time_diff * m_anim_speed, m_is_highlighted);
		m_highlighted_time = max(m_highlighted_time, m_enabled_time);
	}

	void HudButton::onDraw(Renderer2D &out) const {
		FRect rect = this->rect();
		out.addFilledRect(rect, (FColor)backgroundColor());

		Color border_color = borderColor();
		float border_offset = m_button_style == HudButtonStyle::small? 2.5f : 5.0f;
		float offset = lerp(border_offset, 0.0f, m_highlighted_time);
		drawBorder(out, rect, border_color, float2(offset, offset), 20.0f);

		if(!m_label.empty() && m_label_style != HudLabelStyle::disabled) {
			auto halign = 
				m_label_style == HudLabelStyle::left? HAlign::left :
				m_label_style == HudLabelStyle::center? HAlign::center : HAlign::right;
			FRect font_rect = rect.inset(float2(layer_spacing, 0.0f));
			m_font->draw(out, font_rect, {textColor(), textShadowColor(), halign, VAlign::center}, m_label);
		}

		if(m_icon_id)
			out.addFilledRect(rect, s_icons[*m_icon_id].uv_rect, {m_icons_tex, textColor()});
	}
		
	bool HudButton::onInput(const InputEvent &event) {
		if(event.isMouseOverEvent())
			m_is_highlighted = isMouseOver(event);

		if(isGreyed())
			return false;
		
		if( (event.mouseButtonDown(InputButton::left) && isMouseOver(event)) || (m_accelerator && event.keyDown(m_accelerator)) ) {
			onClick();
			return true;
		}

		return false;
	}
		
	void HudButton::onClick() {
		playSound(m_click_sound);
		handleEvent(this, HudEvent::button_clicked, m_id);
	}
		
	Color HudButton::textColor(bool force_enabled) const {
		auto fcolor = lerp((FColor)Color(m_style.enabled_color, 160), FColor(m_style.enabled_color), force_enabled? 1.0f : m_enabled_time);
		fcolor.a *= alpha();
		return (Color)fcolor;
	}
		
	Color HudButton::textShadowColor() const {
		return (Color)mulAlpha(ColorId::black, alpha());
	}

	Color HudButton::backgroundColor() const {
		Color color(m_style.back_color, (int)(alpha() * 127));
		return (Color)desaturate(color, m_greyed_time);
	}
		
	Color HudButton::borderColor() const {
		u8 border_alpha = clamp((int)(255 * alpha() * (0.3f + 0.7f * m_enabled_time * m_highlighted_time)), 0, 255);
		Color color(m_style.border_color, border_alpha);
		return (Color)desaturate(color, m_greyed_time);
	}
		
	void HudButton::setHighlighted(bool is_highlighted, bool animate) {
		m_is_highlighted = is_highlighted;
		if(!animate)
			m_highlighted_time = m_is_highlighted? 1.0f : 0.0f;
	}

	void HudButton::setEnabled(bool is_enabled, bool animate) {
		m_is_enabled = is_enabled;
		if(!animate)
			m_enabled_time = m_is_enabled? 1.0f : 0.0f;
	}
	
	void HudButton::setGreyed(bool is_greyed, bool animate) {
		m_is_greyed = is_greyed;
		if(!animate)
			m_greyed_time = m_is_greyed? 1.0f : 0.0f;
	}
		
	HudClickButton::HudClickButton(const FRect &target_rect, int id)
		:HudButton(target_rect, id), m_is_accelerator(false) { }

	bool HudClickButton::onInput(const InputEvent &event) {
		if(event.isMouseOverEvent())
			m_is_highlighted = isMouseOver(event);

		if(isEnabled()) {
			if(event.mouseButtonUp(InputButton::left) && !m_is_accelerator) {
				setEnabled(false);
				return true;
			}
			if(m_accelerator && event.keyUp(m_accelerator) && m_is_accelerator) {
				m_is_accelerator = false;
				setEnabled(false);
				return true;
			}
		}
		else if(!isGreyed()) {
			m_is_accelerator = m_accelerator && event.keyDown(m_accelerator);
			if(m_is_accelerator || (event.mouseButtonDown(InputButton::left) && isMouseOver(event))) {
				setEnabled(true);
				onClick();
				return true;
			}
		}

		return false;
	}
		
	HudToggleButton::HudToggleButton(const FRect &target_rect, int id)
		:HudButton(target_rect, id) { }
		
	bool HudToggleButton::onInput(const InputEvent &event) {
		if(event.isMouseOverEvent())
			m_is_highlighted = isMouseOver(event);
		
		if(isGreyed())
			return false;

		if((event.mouseButtonDown(InputButton::left) && isMouseOver(event)) || (m_accelerator && event.keyDown(m_accelerator))) {
			setEnabled(!isEnabled());
			onClick();
			return true;
		}

		return false;
	}
		
	HudRadioButton::HudRadioButton(const FRect &target_rect, int id, int group_id)
		:HudButton(target_rect, id), m_group_id(group_id) { }
	
	bool HudRadioButton::onInput(const InputEvent &event) {
		if(event.isMouseOverEvent())
			m_is_highlighted = isMouseOver(event);
		
		if(isGreyed())
			return false;

		bool clicked = (event.mouseButtonDown(InputButton::left) && isMouseOver(event)) || (m_accelerator && event.keyDown(m_accelerator));
		if(clicked && !isEnabled()) {
			setEnabled(true);

			if(parent()) {
				const vector<PHudWidget> &children = parent()->children();
				for(auto &child: children) {
					HudRadioButton *radio = dynamic_cast<HudRadioButton*>(child.get());
					if(radio && radio != this && radio->m_group_id == m_group_id)
						radio->setEnabled(false);
				}
			}
			
			onClick();
			return true;
		}

		return false;
	}
	
}
