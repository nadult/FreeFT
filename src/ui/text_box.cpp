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

#include "text_box.h"

namespace ui {

	TextBox::TextBox(const IRect &rect, const char *text, bool is_centered, Color col)
		:Window(rect, col), m_is_centered(is_centered) {
		m_font = gfx::Font::mgr[s_font_names[0]];
		setText(text);
	}

	void TextBox::setFont(const char *font_name) {
		m_font = gfx::Font::mgr[font_name];
		m_text_extents = m_font->evalExtents(m_text.c_str());
	}

	void TextBox::setText(const char *text) {
		m_text = text;
		m_text_extents = m_font->evalExtents(text);
	}

	void TextBox::drawContents() const {
		int2 pos = (size() - m_text_extents.size()) / 2 - m_text_extents.min;
		if(!m_is_centered)
			pos.x = 5;
		m_font->drawShadowed(pos, Color::white, Color::black, m_text.c_str());
	}

}

