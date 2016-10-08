/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "edit_box.h"

using namespace gfx;

namespace ui {

	EditBox::EditBox(const IRect &rect, int max_size, StringRef label, FColor col)
		:Window(rect, col), m_is_editing(false), m_cursor_pos(0), m_max_size(max_size), m_label(toWideString(label)) {
		m_font = res::getFont(WindowStyle::fonts[0]);
	}

	void EditBox::setText(wstring new_text) {
		m_text = std::move(new_text);
		if((int)m_text.size() > m_max_size)
			m_text.resize(m_max_size);
	}

	void EditBox::drawContents(Renderer2D &out) const {
		int line_height = m_font->lineHeight();

		int2 pos(5, height() / 2 - line_height / 2);
		m_font->draw(out, (float2)pos, {ColorId::white, ColorId::black}, m_label + m_text);

		if(m_is_editing) {
			IRect ext = m_font->evalExtents((m_label + m_text.substr(0, m_cursor_pos)).c_str());
			out.addLine(pos + int2(ext.max.x, 0), pos + int2(ext.max.x, line_height), FColor(ColorId::white, 0.7f));
			out.addRect(IRect(2, 1, width() - 1, height() - 2), FColor(ColorId::white, 0.3f));	
		}
	}
	
	void EditBox::setCursorPos(int2 rect_pos) {
		m_cursor_pos = (int)m_text.size();

		//TODO: speed up
		for(int n = 0; n < (int)m_text.size(); n++) {
			auto text = m_label + m_text.substr(0, n);
			IRect ext = m_font->evalExtents(text.c_str());
			if(ext.max.x + 8 > rect_pos.x) {
				m_cursor_pos = max(0, n);
				break;
			}
		}
	}

	void EditBox::reset(bool is_editing) {
		m_cursor_pos = 0;
		setFocus( (m_is_editing = is_editing) );
		m_old_text = m_text = wstring();
	}

	void EditBox::onInput(const InputState &state) {
		auto mouse_pos = state.mousePos() - clippedRect().min;

		if(!m_is_editing) {
			if(state.isMouseButtonDown(InputButton::left)) {
				setCursorPos(mouse_pos);
				setFocus( (m_is_editing = true) );
				m_old_text = m_text;
			}
		}
		else {
			if(!state.text().empty()) {
				m_text.insert(m_cursor_pos, state.text());
				m_cursor_pos += (int)state.text().size();
			}
			else {
				if(state.isKeyDownAuto(InputKey::right) && m_cursor_pos < (int)m_text.size())
					m_cursor_pos++;
				else if(state.isKeyDownAuto(InputKey::left) && m_cursor_pos > 0)
					m_cursor_pos--;
				else if(state.isKeyDownAuto(InputKey::del) && (int)m_text.size() > m_cursor_pos) {
					printf("erase\n");
					m_text.erase(m_cursor_pos, 1);
				}
				else if(state.isKeyDownAuto(InputKey::backspace) && m_cursor_pos > 0 && !m_text.empty()) {
					printf("erase\n");
					m_text.erase(--m_cursor_pos, 1);
				}
			}

			bool end = state.isKeyDown(InputKey::enter);	
			if(state.isMouseButtonDown(InputButton::left) || end) {
				if(!end && rect().isInside(mouse_pos)) {
					setCursorPos(mouse_pos);
				}
				else {
					setFocus( (m_is_editing = false) );
					if(m_old_text != m_text)
						sendEvent(this, Event::text_modified);
				}
			}
		}
	}

	bool EditBox::onEvent(const Event &event) {
		if(event.type == Event::escape && m_is_editing) {
			setFocus( (m_is_editing = false) );
			m_text = m_old_text;
			return true;
		}

		return false;
	}

}

