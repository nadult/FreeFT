/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "ui/button.h"

using namespace gfx;

namespace ui
{

	Button::Button(IRect rect, const char *text, int id)
		:Window(rect, Color::transparent), m_mouse_press(false), m_id(id), m_is_enabled(true) {
		m_font = Font::mgr[s_font_names[0]];
		ASSERT(m_font);
		setText(text);
	}

	void Button::setText(const char *text) {
		DASSERT(text);
		m_text = text;
		m_text_extents = m_font->evalExtents(text);
		m_text_extents.min.y = 0;
		m_text_extents.max.y = m_font->lineHeight();
	}

	void Button::drawContents() const {
		drawWindow(IRect(int2(0, 0), size()), isMouseOver() && m_is_enabled? Color::gui_light : Color::gui_dark,
					m_mouse_press? -2 : 2);

		int2 rect_center = size() / 2;
		int2 pos = rect_center - m_text_extents.size() / 2 - m_text_extents.min - int2(1, 1);
		if(m_mouse_press)
			pos += int2(2, 2);
		m_font->drawShadowed(pos, m_is_enabled? Color::white : Color::gray, Color::black, m_text.c_str());
	}

	bool Button::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		m_mouse_press = key == 0 && !is_final && m_is_enabled;
		if(key == 0 && m_is_enabled) {
			if(is_final == 1 && localRect().isInside(current))
				sendEvent(this, Event::button_clicked, m_id);
			return true;
		}

		return false;
	}

	void Button::enable(bool do_enable) {
		if(m_is_enabled && !do_enable)
			m_mouse_press = false;
		m_is_enabled = do_enable;
	}

}
