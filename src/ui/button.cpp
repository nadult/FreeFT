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
		Assert(m_font && m_font_texture);
	}

	void Button::drawContents() const {
		m_font_texture->Bind();
		m_font->SetPos({0, (rect().Height() - 20) / 2});
		m_font->SetSize({28, 20});
		m_font->Draw(m_text.c_str());
	}

	void Button::onInput(int2 mouse_pos) {
		setBackgroundColor(IsMouseKeyPressed(0)? s_colors[2] : s_colors[1]);
		if(IsMouseKeyUp(0) && parent() && IRect(int2(0, 0), clippedRect().Size()).IsInside(mouse_pos))
			parent()->onButtonPressed(this);
	}

	void Button::onIdle() {
		setBackgroundColor(s_colors[0]);
	}

}
