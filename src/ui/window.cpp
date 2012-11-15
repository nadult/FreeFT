#include "ui/window.h"
#include "gfx/device.h"

using namespace gfx;

namespace ui
{

	void Window::drawWindow(IRect rect, Color color, int outline) {
		DTexture::bind0();
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

	Window::Window(IRect rect, Color background_color)
		:m_parent(nullptr), m_is_visible(true), m_is_mouse_over(false), m_is_popup(false), m_is_closing(false) {
		m_drag_start = int2(0, 0);
		m_dragging_mode = 0;

		setBackgroundColor(background_color);
		setRect(rect);
	}

	void Window::setBackgroundColor(Color col) {
		m_background_color = col;
	}

	void Window::close(int return_value) {
		m_is_closing = true;
		m_closing_value = return_value;
	}

	void Window::process() {
		Window *popup = nullptr;

		for(int n = 0; n < (int)m_children.size(); n++) {
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
		int2 local_mouse_pos = mouse_pos - m_rect.min;
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
					m_drag_start = local_mouse_pos;
					break;
				}
			}
		}

		int2 focus_point = m_dragging_mode? m_drag_start : local_mouse_pos;
		bool is_handled = false;

		for(int n = 0; n < (int)m_children.size(); n++) {
			Window *child = m_children[n].get();
			if(!child->isVisible())
				continue;

			if(child->rect().isInside(focus_point)) {
				child->process();
				is_handled = true;
			}
			else
				child->onIdle();
		}

		m_is_mouse_over = false;
		if(!is_handled) {
			m_is_mouse_over = m_clipped_rect.isInside(mouse_pos);

			if(escape)
				is_handled = onEscape();


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
					if(isKeyDown(Key_pageup))
						vector.y += rect().height();
					if(isKeyDown(Key_pagedown))
						vector.y -= rect().height();

					setInnerRect(m_inner_rect + vector);
				}

				onInput(local_mouse_pos);
			}
		}

		if(finished_dragging)
			m_dragging_mode = 0;
	}

	void Window::onIdle() {
		for(int n = 0; n < (int)m_children.size(); n++)
			m_children[n]->onIdle();
		m_is_mouse_over = false;
	}

	void Window::draw() const {
		if(!m_parent)
			setScissorTest(true);

		lookAt(-m_clipped_rect.min);
		setScissorRect(m_clipped_rect);

		if(m_background_color.a > 0)
			clear(m_background_color);
		
		drawContents();
		lookAt(-m_clipped_rect.min);

		if(m_has_inner_rect) {
			int2 rsize = m_rect.size();
			int2 isize = m_inner_rect.size();

			Color col1 = Color::gui_dark;
			Color col2 = Color::gui_light;
			col1 = Color(int(col1.r) * 4 / 3, int(col1.g) * 4 / 3, int(col1.b) * 4 / 3, 128);
			col2 = Color(int(col2.r) * 4 / 3, int(col2.g) * 4 / 3, int(col2.b) * 4 / 3, 128);

			DTexture::bind0();
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

	void Window::setRect(const IRect &rect) {
		DASSERT(!m_dragging_mode);
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
		if(onEvent(event))
			return true;

		bool send_up = event.type != Event::window_closed;

		if(send_up) {
			if(m_parent)
				return m_parent? m_parent->sendEvent(event) : false;
		}
		else {
			for(int n = 0; n < (int)m_children.size(); n++)
				if(m_children[n]->sendEvent(event))
					return true;
		}

		return false;
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
