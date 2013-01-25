/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "edit_box.h"
#include "gfx/device.h"

using namespace gfx;

namespace ui {

	EditBox::EditBox(const IRect &rect, int max_size, Color col)
		:Window(rect, col), m_is_editing(false), m_cursor_pos(0), m_last_key(0), m_max_size(max_size) {
		m_font = gfx::Font::mgr[s_font_names[0]];
	}

	void EditBox::setText(const char *text) {
		m_text = text;
		if((int)m_text.size() > m_max_size)
			m_text.resize(m_max_size);
	}

	void EditBox::drawContents() const {
		int line_height = m_font->lineHeight();

		int2 pos(5, height() / 2 - line_height / 2);
		m_font->drawShadowed(pos, Color::white, Color::black, "%s", m_text.c_str());

		DTexture::bind0();
		if(m_is_editing) {
			IRect ext = m_font->evalExtents(m_text.substr(0, m_cursor_pos).c_str());
			drawLine(pos + int2(ext.max.x, 0), pos + int2(ext.max.x, line_height), Color(255, 255, 255, 180));
			drawRect(IRect(2, 1, width() - 1, height() - 2), Color(255, 255, 255, 80));	
		}

	}
	
	void EditBox::setCursorPos(int2 rect_pos) {
		m_cursor_pos = (int)m_text.size();

		//TODO: speed up
		for(int n = 0; n < (int)m_text.size(); n++) {
			string text = m_text.substr(0, n);
			IRect ext = m_font->evalExtents(text.c_str());
			if(ext.max.x + 8 > rect_pos.x) {
				m_cursor_pos = max(0, n);
				break;
			}
		}
	}	

	void EditBox::onInput(int2 mouse_pos) {
		if(!m_is_editing) {
			if(isMouseKeyDown(0)) {
				setCursorPos(mouse_pos);
				setFocus( (m_is_editing = true) );
				m_old_text = m_text;
			}
		}
		else {
			double time = getTime();

			int key =	isKeyPressed(Key_right)? Key_right :
						isKeyPressed(Key_left)? Key_left :
						isKeyPressed(Key_del)? Key_del :
						isKeyPressed(Key_backspace)? Key_backspace : getCharPressed();

			if(key != m_last_key) {
				m_key_down_time = time;
				m_last_key = key;
				onKey(key);
			}

			//TODO: use isKeyDownauto
			if(key == m_last_key && time > m_key_down_time + 0.4f && time > m_on_key_time + 0.025f)
				onKey(key);
		
			bool end = isKeyDown(Key_enter);	
			if(isMouseKeyDown(0) || end) {
				if(!end && rect().isInside(mouse_pos)) {
					setCursorPos(mouse_pos);
					m_last_key = 0;
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

	void EditBox::onKey(int key) {
		if(key == Key_right && m_cursor_pos < (int)m_text.size())
			m_cursor_pos++;
		else if(key == Key_left && m_cursor_pos > 0)
			m_cursor_pos--;
		else if(key == Key_del && (int)m_text.size() > m_cursor_pos)
			m_text.erase(m_cursor_pos, 1);
		else if(key == Key_backspace && m_cursor_pos > 0 && !m_text.empty())
			m_text.erase(--m_cursor_pos, 1);
		else if(key >= 32 && key < 128 && (int)m_text.size() < m_max_size)
			m_text.insert(m_cursor_pos++, 1, key);
		else
			return;

		m_on_key_time = getTime();
	}

}

