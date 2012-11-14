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
		:m_parent(nullptr), m_is_visible(true), m_is_mouse_over(false) {
		m_drag_start = int2(0, 0);
		m_dragging_mode = 0;

		setBackgroundColor(background_color);
		setRect(rect);
	}

	void Window::setBackgroundColor(Color col) {
		m_background_color = col;
	}

	void Window::handleInput() {
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
				child->handleInput();
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

	void Window::addChild(PWindow &&child) {
		DASSERT(child);
		child->m_parent = this;
		child->updateRects();
		m_children.push_back(std::move(child));
	}

	void Window::addPopup(PWindow &&popup) {
		if(m_parent)
			m_parent->addPopup(std::move(popup));
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

	void Window::onButtonPressed(Button *button) {
		if(m_parent)
			m_parent->onButtonPressed(button);
	}
		
	void Window::onListElementClicked(ListView *list_view, int id) {
		if(m_parent)
			m_parent->onListElementClicked(list_view, id);
	}
	
	void Window::onClosePopup(Window *popup, int ret) {
		if(m_parent)
			m_parent->onClosePopup(popup, ret);
	}
		
	void Window::onEvent(Window *source, int event, int value) {
		if(m_parent)
			m_parent->onEvent(source, event, value);
	}
	
	bool Window::isMouseOver() const {
		return m_is_mouse_over;
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
	
	void MainWindow::handleInput() {
		if(m_popups.empty())
			Window::handleInput();
		else
			m_popups.back()->handleInput();
	}

	void MainWindow::draw() const {
		Window::draw();
		for(int n = 0; n < (int)m_popups.size(); n++)
			m_popups[n]->draw();
	}

	void MainWindow::addPopup(PWindow &&popup) {
		DASSERT(popup);
		popup->m_parent = this;
		popup->updateRects();
		m_popups.push_back(std::move(popup));
	}

	void MainWindow::onClosePopup(Window *popup, int ret) {
		for(int n = 0; n < m_popups.size(); n++)
			if(m_popups[n] == popup) {
				m_popups.erase(m_popups.begin() + n);
				break;
			}
	}

}
