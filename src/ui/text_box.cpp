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

