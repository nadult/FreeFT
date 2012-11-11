#include "ui/button.h"

using namespace gfx;

namespace ui
{
	static const Color s_colors[3] = {
		Color(128, 128, 128),
		Color(140, 140, 140),
		Color(180, 180, 180) };


	Button::Button(IRect rect, const char *text) :Window(rect, s_colors[0]), m_text(text) {
		m_font = Font::mgr["font1"];
		m_font_texture = Font::tex_mgr["font1"];
		ASSERT(m_font && m_font_texture);
	}

	void Button::drawContents() const {
		m_font_texture->bind();
		m_font->setPos({0, (rect().height() - 20) / 2});
		m_font->setSize({28, 20});
		m_font->draw(m_text.c_str());
	}

	void Button::onInput(int2 mouse_pos) {
		setBackgroundColor(isMouseKeyPressed(0)? s_colors[2] : s_colors[1]);
		if(isMouseKeyUp(0) && parent() && IRect(int2(0, 0), clippedRect().size()).isInside(mouse_pos))
			parent()->onButtonPressed(this);
	}

	void Button::onIdle() {
		setBackgroundColor(s_colors[0]);
	}

}
