#include "text_box.h"

namespace ui {

	TextBox::TextBox(const IRect &rect, const char *text, bool is_centered, Color col)
		:Window(rect, col), m_is_centered(is_centered) {
		m_font = gfx::Font::mgr["times_24"];
		setText(text);
	}

	void TextBox::setText(const char *text) {
		m_text = text;
		IRect extents = m_font->evalExtents(text);
		m_text_size = extents.size();
	}

	void TextBox::drawContents() const {
		int2 rsize = rect().size();
		int2 pos(m_is_centered? rsize.x / 2 - m_text_size.x / 2 : 5, rsize.y / 2 - m_text_size.y);
		m_font->drawShadowed(pos, Color::white, Color::black, m_text.c_str());
	}

}

