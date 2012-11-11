#include "ui/window.h"
#include "gfx/device.h"

using namespace gfx;

namespace ui
{

	Window::Window(IRect rect, Color background_color)
		:m_parent(nullptr), m_background_color(background_color), m_is_visible(true) {
			m_drag_start = int2(0, 0);
			m_dragging_mode = 0;

			setRect(rect);
		}

	void Window::handleInput() {
		int2 mouse_pos = getMousePos() - m_clipped_rect.min;
		bool finished_dragging = false;
		bool escape = isKeyDown(Key_esc);

		if(m_dragging_mode) {
			if(!isMouseKeyPressed(m_dragging_mode - 1) || escape)
				finished_dragging = true;
		}
		else {
			for(int k = 0; k < 3; k++) {
				if(isMouseKeyDown(k)) {
					m_dragging_mode = k + 1;
					m_drag_start = mouse_pos;
					break;
				}
			}
		}

		int2 focus_point = m_dragging_mode? m_drag_start : mouse_pos;
		bool is_handled = false;

		for(int n = 0; n < (int)m_children.size(); n++) {
			Window *child = m_children[n].get();
			if(!child->isVisible())
				continue;

			if(child->rect().isInside(focus_point)) {
				child->handleInput();
				is_handled = true;
			}
			else
				child->onIdle();
		}

		if(!is_handled) {
			if(escape)
				is_handled = onEscape();
			if(m_dragging_mode && !is_handled) {
				is_handled = onMouseDrag(m_drag_start, mouse_pos, m_dragging_mode - 1, finished_dragging);
				if(!is_handled)
					is_handled = onMouseClick(mouse_pos, m_dragging_mode - 1, finished_dragging);
			}
			if(!is_handled)
				onInput(mouse_pos);
		}

		if(finished_dragging)
			m_dragging_mode = 0;
	}

	void Window::onIdle() {
		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->onIdle();
	}

	void Window::draw() const {
		if(!m_parent)
			setScissorTest(true);

		lookAt(-m_clipped_rect.min);
		setScissorRect(m_clipped_rect);

		if(m_background_color.a > 0)
			clear(m_background_color);

		drawContents();

		for(int n = 0; n < (int)m_children.size(); n++)
			if(m_children[n]->isVisible())
				m_children[n]->draw();
		
		if(!m_parent)
			setScissorTest(false);	
	}

	void Window::addChild(PWindow &&child) {
		DASSERT(child);
		child->m_parent = this;
		child->updateRects();
		m_children.push_back(std::move(child));
	}
		
	void Window::setRect(IRect rect) {
		DASSERT(!m_dragging_mode);
		m_rect = rect;
		updateRects();
	}

	void Window::onButtonPressed(Button *button) {
		if(parent())
			parent()->onButtonPressed(button);
	}

	void Window::updateRects() {
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
