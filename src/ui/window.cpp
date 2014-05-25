/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/window.h"
#include "gfx/device.h"
#include <cstdlib>

using namespace gfx;

namespace ui
{

	/*
	Color WindowStyle::gui_dark  (0x25, 0x6f, 0xfe);
	Color WindowStyle::gui_medium(0x38, 0x77, 0xfe);
	Color WindowStyle::gui_light (0x4c, 0x86, 0xfe);
	Color WindowStyle::gui_popup (0x60, 0xa0, 0xff); */
	
	Color WindowStyle::gui_dark  (0.10f, 0.50f, 0.10f);
	Color WindowStyle::gui_medium(0.15f, 0.60f, 0.15f);
	Color WindowStyle::gui_light (0.20f, 0.70f, 0.20f);
	Color WindowStyle::gui_popup (0.30f, 0.80f, 0.30f);

	const char *WindowStyle::fonts[3] = {
		"liberation_16",
		"liberation_24",
		"liberation_32",
	};

	void Window::drawWindow(IRect rect, Color color, int outline) {
		DTexture::unbind();
		float3 fcolor = (float3)color;
		float falpha = float(color.a) * (1.0f / 255.0f);

		Color lighter(fcolor * 1.2f, falpha);
		Color darker(fcolor * 0.8f, falpha);

		int aoutline = abs(outline);

		if(outline) {
			int2 hsize(rect.width(), aoutline);
			int2 vsize(aoutline, rect.height());

			drawQuad(rect.min, hsize, outline < 0?darker : lighter);
			drawQuad(rect.min, vsize, outline < 0?darker : lighter);

			drawQuad(int2(rect.min.x, rect.max.y - aoutline), hsize, outline < 0?lighter : darker);
			drawQuad(int2(rect.max.x - aoutline, rect.min.y) , vsize, outline < 0?lighter : darker);
		}

		drawQuad(rect.min + int2(aoutline, aoutline), rect.size() - int2(aoutline, aoutline) * 2, color);
	}

	Window::Window(const IRect &rect, Color background_color)
		:m_parent(nullptr), m_is_visible(true), m_is_popup(false), m_is_closing(false),
			m_is_focused(false), m_has_hard_focus(false), m_is_mouse_over(false) {
		m_drag_start = int2(0, 0);
		m_dragging_mode = 0;

		setBackgroundColor(background_color);
		setRect(rect);
	}

	void Window::setBackground(gfx::PTexture background) {
		m_background = background;
	}

	void Window::setBackgroundColor(Color col) {
		m_background_color = col;
	}

	bool Window::isFocused() const {
		return m_is_focused && (!parent() || parent()->isFocused());
	}

	bool Window::isMouseOver() const {
		return isFocused() && m_is_mouse_over;
	}

	void Window::close(int return_value) {
		m_is_closing = true;
		m_closing_value = return_value;
	}

	void Window::setFocus(bool set) {
		if(set == m_has_hard_focus)
			return;

		m_has_hard_focus = set;
		if(!m_is_popup && parent())
			parent()->setFocus(set);
	}

	void Window::process() {
		Window *popup = nullptr;
		m_is_focused = true;

		for(int n = 0; n < (int)m_children.size(); n++) {
			m_children[n]->m_is_focused = false;
			m_children[n]->m_is_mouse_over = false;

			if(m_children[n]->m_is_closing) {
				PWindow window = m_children[n];
				m_children.erase(m_children.begin() + n);
				sendEvent(window.get(), Event::window_closed, m_children[n]->m_closing_value); 
				n--;
			}
			else if(m_children[n]->m_is_popup)
				popup = m_children[n].get();
		}

		if(popup) {
			popup->process();
			return;
		}

		int2 mouse_pos = getMousePos();
		int2 local_mouse_pos = mouse_pos - m_clipped_rect.min;
		int finished_dragging = 0;
		bool escape = isKeyDown(Key_esc);

		if(m_dragging_mode) {
			if(!isMouseKeyPressed(m_dragging_mode - 1) || escape)
				finished_dragging = escape? -1 : 1;
		}
		else {
			for(int k = 0; k < 3; k++) {
				if(isMouseKeyDown(k)) {
					m_dragging_mode = k + 1;
					m_drag_start = local_mouse_pos;
					break;
				}
			}
		}


		int2 focus_point = m_dragging_mode? m_drag_start : local_mouse_pos;
		bool is_handled = false;

		for(int n = (int)m_children.size() - 1; n >= 0; n--) {
			Window *child = m_children[n].get();
			if(child->isVisible() && m_has_hard_focus == child->m_has_hard_focus) {
				if(m_has_hard_focus || child->rect().isInside(focus_point)) {
					child->m_is_mouse_over = child->clippedRect().isInside(mouse_pos);
					child->process();
					is_handled = true;
					break;
				}
			}
		}

		if(!is_handled) {
			if(m_dragging_mode && !is_handled) {
				if(m_has_inner_rect && m_dragging_mode - 1 == 2) {
					setInnerRect(m_inner_rect + getMouseMove());
					is_handled = true;
				}
				if(!is_handled)
					is_handled = onMouseDrag(m_drag_start, local_mouse_pos, m_dragging_mode - 1, finished_dragging);
				if(!is_handled)
					is_handled = onMouseClick(local_mouse_pos, m_dragging_mode - 1, finished_dragging);
			}
			if(!is_handled) {
				if(m_has_inner_rect) {
					int wheel = getMouseWheelMove();
					int2 vector(0, 0);

					if(wheel)
						vector.y += wheel * rect().height() / 8;
					if(isKeyDownAuto(Key_pageup, 2))
						vector.y += rect().height();
					if(isKeyDownAuto(Key_pagedown, 2))
						vector.y -= rect().height();

					setInnerRect(m_inner_rect + vector);
				}

				onInput(local_mouse_pos);
			}
		}
		
		if(escape && (!m_parent || m_is_popup) && !m_dragging_mode) // sending Event::escape only from main window
			sendEvent(this, Event::escape); //TODO: send only to focused windows

		if(finished_dragging)
			m_dragging_mode = 0;
	}

	void Window::draw() const {
		if(!m_parent)
			setScissorTest(true);

		lookAt(-m_clipped_rect.min);
		setScissorRect(m_clipped_rect);

		if(m_background_color.a > 0 && !(m_background && m_background->dimensions() == m_rect.size())) {
			if(m_background_color.a == 255)
				clear(m_background_color);
			else {
				DTexture::unbind();
				drawQuad(m_clipped_rect.min, m_clipped_rect.max, m_background_color);
			}
		}

		if(m_background) {
			m_background->bind();
			drawQuad(int2(0, 0), m_background->dimensions());
		}
		
		drawContents();
		lookAt(-m_clipped_rect.min);

		if(m_has_inner_rect) {
			int2 rsize = m_rect.size();
			int2 isize = m_inner_rect.size();

			Color col1 = WindowStyle::gui_dark;
			Color col2 = WindowStyle::gui_light;
			col1 = Color(int(col1.r) * 4 / 5, int(col1.g) * 4 / 5, int(col1.b) * 4 / 5, 128);
			col2 = Color(int(col2.r) * 4 / 3, int(col2.g) * 4 / 3, int(col2.b) * 4 / 3, 128);

			// TODO: minimum size of progress bar, coz sometimes its almost invisible
			DTexture::unbind();
			if(isize.x > rsize.x) {
				float divisor = 1.0f / float(isize.x);
				float spos = float(0       - m_inner_rect.min.x) * divisor;
				float epos = float(rsize.x - m_inner_rect.min.x) * divisor;

				drawQuad(int2(0, rsize.y - 5), int2(rsize.x, 5), col1);
				drawQuad(int2(spos * rsize.x, rsize.y - 5), int2(epos * rsize.x, 5), col2);
			}
			if(isize.y > rsize.y) {
				float divisor = 1.0f / float(isize.y);
				float spos = float(0       - m_inner_rect.min.y) * divisor;
				float epos = float(rsize.y - m_inner_rect.min.y) * divisor;

				drawQuad(int2(rsize.x - 5, 0), int2(5, rsize.y), col1);
				drawQuad(int2(rsize.x - 5, spos * rsize.y), int2(5, (epos - spos) * rsize.y), col2);
			}
		}

		for(int n = 0; n < (int)m_children.size(); n++)
			if(m_children[n]->isVisible())
				m_children[n]->draw();
		
		if(!m_parent)
			setScissorTest(false);	
	}

	void Window::attach(PWindow child, bool as_popup) {
		DASSERT(child);
		child->m_parent = this;
		child->m_is_popup = as_popup;
		child->updateRects();
		m_children.push_back(std::move(child));
	}

	void Window::detach(PWindow child) {
		for(int n = 0; n < (int)m_children.size(); n++)
			if(m_children[n] == child) {
				m_children.erase(m_children.begin() + n);
				break;
			}
	}

	void Window::setRect(const IRect &rect) {
		DASSERT(!m_dragging_mode);
		DASSERT(!rect.isEmpty());

		m_rect = rect;
		setInnerRect(IRect({0, 0}, m_rect.size()));
		updateRects();
	}

	void Window::setInnerRect(const IRect &rect) {
		int2 isize = rect.size();
		m_inner_rect.min = max(min(rect.min, {0, 0}), min(-isize + m_rect.size(), {0, 0}));
		m_inner_rect.max = m_inner_rect.min + isize;
		m_has_inner_rect = isize.x > m_rect.width() || isize.y > m_rect.height();
	}

	void Window::setInnerOffset(const int2 &offset) {
		setInnerRect(IRect(-offset, -offset + m_inner_rect.size()));
	}
		
	bool Window::sendEvent(const Event &event) {
		bool send_up = event.type != Event::window_closed && event.type != Event::escape;
		if(event.type == Event::escape && !m_is_focused)
			return false;

		if(send_up) {
			if(onEvent(event))
				return true;

			if(m_parent)
				return m_parent? m_parent->sendEvent(event) : false;
		}
		else {
			for(int n = (int)m_children.size() - 1; n >= 0; n--)
				if(m_children[n]->sendEvent(event))
					return true;

			if(onEvent(event))
				return true;
		}

		return false;
	}

	void Window::updateRects() {
		DASSERT(m_rect.min.x >= 0 && m_rect.min.y >= 0);
		m_clipped_rect = m_rect;

		if(m_parent) {
			IRect parent_rect = m_parent->m_clipped_rect;
			m_clipped_rect += parent_rect.min;
			m_clipped_rect.max = min(m_clipped_rect.max, parent_rect.max);
		}

		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->updateRects();
	}


}
