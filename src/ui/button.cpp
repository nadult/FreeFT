// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "ui/button.h"

namespace ui {

Button::Button(IRect rect, const char *text, int id)
	: Window(rect, ColorId::transparent), m_mouse_press(false), m_id(id), m_is_enabled(true),
	  m_font(res::getFont(WindowStyle::fonts[0])) {
	setText(text);
}

void Button::setText(const char *text) {
	DASSERT(text);
	m_text = text;
	m_text_extents = m_font.evalExtents(text);
	m_text_extents = {m_text_extents.x(), 0, m_text_extents.ex(), m_font.lineHeight()};
}

void Button::drawContents(Canvas2D &out) const {
	drawWindow(out, IRect(int2(0, 0), size()),
			   isMouseOver() && m_is_enabled ? WindowStyle::gui_light : WindowStyle::gui_dark,
			   m_mouse_press ? -2 : 2);

	int2 rect_center = size() / 2;
	int2 pos = rect_center - m_text_extents.size() / 2 - m_text_extents.min() - int2(1, 1);
	if(m_mouse_press)
		pos += int2(2, 2);
	m_font.draw(out, (float2)pos, {m_is_enabled ? ColorId::white : ColorId::gray, ColorId::black},
				m_text);
}

bool Button::onMouseDrag(const InputState &, int2 start, int2 current, int key, int is_final) {
	m_mouse_press = key == 0 && !is_final && m_is_enabled;
	if(key == 0 && m_is_enabled) {
		if(is_final == 1 && localRect().containsCell(current))
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
