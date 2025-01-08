// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "progress_bar.h"

namespace ui {

ProgressBar::ProgressBar(const IRect &rect, bool is_horizontal)
	: Window(rect, ColorId::transparent), m_bar_size(0.1f), m_pos(0.0f), m_mouse_press(false),
	  m_is_horizontal(is_horizontal) {}

void ProgressBar::setBarSize(float size) {
	DASSERT(size >= 0.0f && size <= 1.0f);
	m_bar_size = size;
}

void ProgressBar::setPos(float pos) {
	DASSERT(pos >= 0.0f && pos <= 1.0f);
	m_pos = pos;
}

void ProgressBar::setText(const char *text) { m_text = text; }

float ProgressBar::evalBarSize() const {
	float rsize = m_is_horizontal ? width() : height();
	float bar_size = max(min(16.0f, rsize), m_bar_size * rsize);
	return bar_size / rsize;
}

IRect ProgressBar::evalBarPos() const {
	int2 rect_size = size();
	float bar_size = evalBarSize();
	float pos = m_pos * (1.0f - bar_size);

	return m_is_horizontal ?
			   IRect(rect_size.x * pos, 0, rect_size.x * (pos + bar_size), rect_size.y) :
			   IRect(0, rect_size.y * pos, rect_size.x, rect_size.y * (pos + bar_size));
}

void ProgressBar::drawContents(Canvas2D &out) const {
	//TODO: fixme
	/*
		drawWindow(IRect(int2(0, 0), size()), WindowStyle::gui_medium, 1);
		drawWindow(evalBarPos(), isMouseOver()? WindowStyle::gui_light : WindowStyle::gui_dark, m_mouse_press? -2 : 2);

		if(!m_text.empty()) {
			auto &font = res::getFont(WindowStyle::fonts[0]);
			IRect extents = font.evalExtents(m_text.c_str());
			int2 pos = (size() - extents.size()) / 2 - extents.min();
			font.draw((float2)pos, {ColorId::white, ColorId::black}, m_text);
		}*/
}

bool ProgressBar::onMouseDrag(const InputState &, int2 start, int2 current, int key, int is_final) {
	if(key != 0)
		return false;

	float bar_size = evalBarSize();
	float divisor = 1.0f / float(m_is_horizontal ? width() : height());

	if(start == current) {
		IRect bar_rect = evalBarPos();
		if(!bar_rect.containsCell(start)) {
			float tpos = float(m_is_horizontal ? start.x : start.y) * divisor;
			tpos -= (0.5f - tpos) * bar_size;
			float old_pos = m_pos;
			m_pos = clamp(tpos, 0.0f, 1.0f);
			if(m_pos != old_pos)
				sendEvent(this, Event::progress_bar_moved);
		}
		m_start_pos = m_pos;
	}

	float vec =
		(m_is_horizontal ? current.x - start.x : current.y - start.y) / (1.0f - bar_size) * divisor;
	float old_pos = m_pos;
	m_pos = clamp(m_start_pos + vec, 0.0f, 1.0f);
	if(m_pos != old_pos)
		sendEvent(this, Event::progress_bar_moved);
	m_mouse_press = !is_final;

	return true;
}

}
