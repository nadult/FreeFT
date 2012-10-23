#include "ui/window.h"
#include "gfx/device.h"

using namespace gfx;

namespace ui
{

	Window::Window(IRect rect, Color background_color)
		:m_parent(nullptr), m_background_color(background_color) {
			m_drag_start = int2(0, 0);
			m_dragging_mode = 0;
			m_is_dragging = false;

			setRect(rect);
		}

	void Window::handleInput() {
		int2 mouse_pos = GetMousePos() - m_clipped_rect.min;
		bool finished_dragging = false;
		int click = 0;

		if(m_dragging_mode) {
			if(mouse_pos != m_drag_start)
				m_is_dragging = true;

			if(!IsMouseKeyPressed(m_dragging_mode - 1))
				finished_dragging = true;
		}
		else {
			for(int k = 0; k < 3; k++)
				if(IsMouseKeyDown(k)) {
					m_dragging_mode = click = k + 1;
					m_drag_start = mouse_pos;
				}
		}

		int2 focus_point = m_dragging_mode? m_drag_start : mouse_pos;
		bool is_handled = false;

		for(int n = 0; n < (int)m_children.size(); n++) {
			Window *child = m_children[n].get();

			if(child->rect().IsInside(focus_point)) {
				child->handleInput();
				is_handled = true;
			}
			else
				child->onIdle();
		}

		if(!is_handled) {
			if(m_is_dragging)
				is_handled = onMouseDrag(m_drag_start, mouse_pos, m_dragging_mode - 1, finished_dragging);
			else if(click)
				is_handled = onMouseClick(mouse_pos, click - 1);

			if(!is_handled)
				onInput(mouse_pos);
		}

		if(finished_dragging) {
			m_dragging_mode = 0;
			m_is_dragging = false;
		}
	}

	void Window::onIdle() {
		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->onIdle();
	}

	void Window::draw() const {
		if(!m_parent)
			SetScissorTest(true);

		LookAt(-m_clipped_rect.min);
		SetScissorRect(m_clipped_rect);

		if(m_background_color.a > 0)
			Clear(m_background_color);

		drawContents();

		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->draw();
		
		if(!m_parent)
			SetScissorTest(false);	
	}

	void Window::addChild(PWindow &&child) {
		DAssert(child);
		child->m_parent = this;
		child->updateRects();
		m_children.push_back(std::move(child));
	}
		
	void Window::setRect(IRect rect) {
		DAssert(!m_dragging_mode);
		m_rect = rect;
		updateRects();
	}

	void Window::updateRects() {
		m_clipped_rect = m_rect;
		if(m_parent) {
			IRect parent_rect = m_parent->m_clipped_rect;
			m_clipped_rect += parent_rect.min;
			m_clipped_rect.max = Min(m_clipped_rect.max, parent_rect.max);
		}

		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->updateRects();
	}

}
