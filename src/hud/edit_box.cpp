// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/edit_box.h"

#include "game/actor.h"
#include "game/world.h"
#include "gfx/drawing.h"
#include <fwk/gfx/font.h>

namespace hud {

	HudEditBox::HudEditBox(const FRect &rect, int max_size, EditMode mode, int id)
		:HudButton(rect, id), m_cursor_pos(0), m_max_size(max_size), m_mode(mode) {
		setLabelStyle(HudLabelStyle::disabled);
	}

	void HudEditBox::setText(const string &text) {
		m_text = text;
		m_old_text = m_text;
		m_cursor_pos = min(m_cursor_pos, (int)m_text.size());
	}

	void HudEditBox::onUpdate(double time_diff) {
		HudButton::onUpdate(time_diff);
		m_show_time = m_is_visible? m_show_time + time_diff : 0.0f;
	}
		
	void HudEditBox::onInputFocus(bool is_focused) {
		setEnabled(is_focused);
	}

	bool HudEditBox::onInput(const InputEvent &event) {
		bool handled = false;

		if(m_mode == mode_console && event.keyDown('`'))
			return false;

		if(event.isMouseOverEvent())
			m_is_highlighted = isMouseOver(event);

		if(!isEnabled() && event.mouseButtonDown(InputButton::left) && !isGreyed() && isMouseOver(event)) {
			setInputFocus(true);
			handled = true;
			m_old_text = m_text;
		}

		if(isEnabled()) {
			if(event.keyDown(InputKey::enter) || event.keyDown(InputKey::esc) ||
			  (event.mouseButtonDown(InputButton::left) && !isMouseOver(event))) {
				if(event.keyDown(InputKey::esc))
					m_text = m_old_text;
				setInputFocus(false);
				handleEvent(this, HudEvent::text_modified, m_id);
				handled = true;
			}
			else if(isEnabled()) {
				if(event.type() == InputEventType::key_char) {
					int key = event.keyChar();
					if(isValidChar(key) && (int)m_text.size() < m_max_size)
						m_text.insert(m_cursor_pos++, 1, m_mode == mode_locase_nick? tolower(key) : key);
				}
				else if(event.keyDown(InputKey::end))
					m_cursor_pos = (int)m_text.size();
				else if(event.keyDown(InputKey::home))
					m_cursor_pos = 0;
				else if(event.keyDownAuto(InputKey::right) && m_cursor_pos < (int)m_text.size())
					m_cursor_pos++;
				else if(event.keyDownAuto(InputKey::left) && m_cursor_pos > 0)
					m_cursor_pos--;
				else if(event.keyDownAuto(InputKey::del) && (int)m_text.size() > m_cursor_pos)
					m_text.erase(m_cursor_pos, 1);
				else if(event.keyDownAuto(InputKey::backspace) && m_cursor_pos > 0 && !m_text.empty())
					m_text.erase(--m_cursor_pos, 1);
			}
			if(event.mouseButtonPressed(InputButton::left) && isMouseOver(event)) {
				setCursorPos(float2(event.mousePos() - (int2)rect().min()));
			}
		}

		return handled;
	}
		
	bool HudEditBox::isValidChar(int key) {
		if(m_mode == mode_normal || m_mode == mode_console)
		   return key >= 32 && key < 128;
		else if(m_mode == mode_nick || m_mode == mode_locase_nick)
			return (key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key == '-' || key == '_';

		return false;
	}

	void HudEditBox::setCursorPos(const float2 &rect_pos) {
		m_cursor_pos = (int)m_text.size();

		//TODO: speed up
		for(int n = 0; n < (int)m_text.size(); n++) {
			FRect ext = evalExtents(m_text.substr(0, n));
			if(ext.ex() + 5.0f > rect_pos.x - layer_spacing) {
				m_cursor_pos = max(0, n);
				break;
			}
		}
	}
		
	const FRect HudEditBox::evalExtents(const string &text) const {
		return FRect(m_font->evalExtents(m_label + text));
	}

	void HudEditBox::onDraw(Renderer2D &out) const {
		HudButton::onDraw(out);

		int line_height = m_font->lineHeight();

		FRect target_rect = m_font->draw(out, rect() + float2(layer_spacing, 0.0f), {textColor(), textShadowColor(), HAlign::left, VAlign::center},
				format("%%", m_label, m_text));

		int tick = ((int)(m_show_time / 0.4)) % 3;
		if(isEnabled() && tick <= 1) {
			FRect ext = evalExtents(m_text.substr(0, m_cursor_pos));
			int2 pos = (int2)target_rect.min() + int2(ext.ex() + 2.0f, 0);
			out.addLine(pos, pos + int2(0, line_height), Color(255, 255, 255, 180));
		}

	}

}
