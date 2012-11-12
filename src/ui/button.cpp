#include "ui/button.h"

using namespace gfx;

namespace ui
{
	static const Color s_colors[3] = {
		Color(128, 128, 128),
		Color(140, 140, 140),
		Color(180, 180, 180) };


	Button::Button(IRect rect, const char *text) :Window(rect, s_colors[0]), m_is_being_pressed(0) {
		m_font = Font::mgr["times_24"];
		ASSERT(m_font);
		setText(text);
	}

	void Button::setText(const char *text) {
		m_text = text;
		m_text_extents = m_font->evalExtents(text);
	}

	void Button::drawContents() const {
		int2 rect_center = rect().size() / 2;
		int2 pos = rect_center - m_text_extents.size() / 2 - m_text_extents.min - int2(1, 1);
		if(m_is_being_pressed)
			pos += int2(2, 2);
		m_font->drawShadowed(pos, Color::white, Color::black, m_text.c_str());
	}

	void Button::onInput(int2 mouse_pos) {
		m_is_being_pressed = isMouseKeyPressed(0);
		setBackgroundColor(m_is_being_pressed? s_colors[2] : s_colors[1]);
		if(isMouseKeyUp(0) && parent() && IRect(int2(0, 0), clippedRect().size()).isInside(mouse_pos))
			parent()->onButtonPressed(this);
	}

	void Button::onIdle() {
		m_is_being_pressed = false;
		setBackgroundColor(s_colors[0]);
	}

}
