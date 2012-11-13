#include "ui/button.h"

using namespace gfx;

namespace ui
{

	Button::Button(IRect rect, const char *text)
		:Window(rect, Color::transparent), m_mouse_press(false) {
		m_font = Font::mgr["times_16"];
		ASSERT(m_font);
		setText(text);
	}

	void Button::setText(const char *text) {
		m_text = text;
		m_text_extents = m_font->evalExtents(text);
		m_text_extents.min.y = 0;
		m_text_extents.max.y = m_font->lineHeight();
	}

	void Button::drawContents() const {
		drawWindow(IRect(int2(0, 0), rect().size()), isMouseOver()? Color::gui_light : Color::gui_dark,
					m_mouse_press? -2 : 2);

		int2 rect_center = rect().size() / 2;
		int2 pos = rect_center - m_text_extents.size() / 2 - m_text_extents.min - int2(1, 1);
		if(m_mouse_press)
			pos += int2(2, 2);
		m_font->drawShadowed(pos, Color::white, Color::black, m_text.c_str());
	}

	void Button::onInput(int2 mouse_pos) {
		m_mouse_press = isMouseKeyPressed(0);
		if(isMouseKeyUp(0) && parent() && isMouseOver())
			parent()->onButtonPressed(this);
	}

}
