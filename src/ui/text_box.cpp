/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "text_box.h"

namespace ui {

	TextBox::TextBox(const IRect &rect, const char *text, bool is_centered, Color col)
		:Window(rect, col), m_is_centered(is_centered) {
		m_font = res::getFont(WindowStyle::fonts[0]);
		setText(text);
	}

	void TextBox::setFont(const char *font_name) {
		m_font = res::getFont(font_name);
		m_text_extents = m_font->evalExtents(m_text.c_str());
	}

	void TextBox::setText(const char *text) {
		m_text = text;
		m_text_extents = m_font->evalExtents(text);
	}

	void TextBox::drawContents(Renderer2D &out) const {
		int2 pos = (size() - m_text_extents.size()) / 2 - m_text_extents.min;
		if(!m_is_centered)
			pos.x = 5;
		m_font->draw(out, pos, {Color::white, Color::black}, m_text);
	}

}

