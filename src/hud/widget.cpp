/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/widget.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/opengl.h"

using namespace gfx;

namespace hud {


	HudWidget::HudWidget(const FRect &rect)
		:m_parent(nullptr), m_input_focus(nullptr), m_rect(rect), m_visible_time(1.0), m_anim_speed(10.0f),
		m_is_visible(true), m_needs_layout(true) {
		setStyle(defaultStyle());
	}

	HudWidget::~HudWidget() { }
		
	void HudWidget::setInputFocus(bool is_focused) {
		if( m_parent && ((m_parent->m_input_focus == this) != is_focused) ) {
			HudWidget *top = m_parent;
			while(top->m_parent)
				top = top->m_parent;

			HudWidget *old_focus = top->m_input_focus;
			while(old_focus) {
				old_focus->m_parent->m_input_focus = nullptr;
				old_focus = old_focus->m_input_focus;
			}

			if(is_focused) {
				HudWidget *current = m_parent, *focus = this;
				while(current) {
					current->m_input_focus = focus;
					focus = current;
					current = current->m_parent;
				}
			}
		}

		onInputFocus(is_focused);
	}
	
	bool HudWidget::handleInput(const io::InputEvent &event) {
		if(!m_is_visible)
			if(!isOneOf(event.type(), io::InputEvent::key_up, io::InputEvent::mouse_key_up, io::InputEvent::mouse_over))
				return false;

		io::InputEvent cevent = event;
		cevent.translate(-rect().min);
		bool focus_handled = false;

		HudWidget *handled = nullptr;
		if(m_input_focus && !cevent.mouseOver()) {
			handled = m_input_focus;
			if(m_input_focus->handleInput(cevent))
				return true;
		}
		
		if(onInput(event))
			return true;

		for(auto &child: m_children)
			if(child.get() != handled)
				if(child->handleInput(cevent))
					return true;
		return false;
	}
		
	bool HudWidget::handleEvent(const HudEvent &event) {
		if(onEvent(event))
			return true;
		return m_parent? m_parent->handleEvent(event) : false;
	}
		
	void HudWidget::fitRectToChildren(const float2 &min_size, bool only_visible) {
		float2 max_pos = min_size;
		for(auto &child : m_children) {
			if(only_visible && !child->isVisible())
				continue;

			float2 child_max = child->rect().max;
			max_pos.x = max(max_pos.x, child_max.x + spacing);
			max_pos.y = max(max_pos.y, child_max.y + spacing);
		}

		m_rect.max = m_rect.min + max_pos;
	}
	
	void HudWidget::setStyle(const HudStyle &style) {
		m_style = style;
		m_font = Font::mgr[style.font_name];
		m_big_font = Font::mgr[style.big_font_name];

		for(auto &child: m_children)
			child->setStyle(style);
	}
		
	Ptr<HudWidget> HudWidget::detach(HudWidget *widget) {
		DASSERT(widget->m_parent == this);
		Ptr<HudWidget> out;

		setInputFocus(false);
		widget->m_parent = nullptr;

		for(auto child = m_children.begin(); child != m_children.end(); ++child) {
			if(child->get() == widget) {
				out = std::move(*child);
				m_children.erase(child);
				break;
			}
		}

		return std::move(out);
	}
		
	void HudWidget::attach(Ptr<HudWidget> child) {
		DASSERT(child);
		child->setStyle(m_style);
		child->m_parent = this;
		m_children.push_back(std::move(child));
	}
		
	void HudWidget::needsLayout() {
		m_needs_layout = true;
		if(m_parent)
			m_parent->needsLayout();
	}
		
	void HudWidget::layout() {
		if(!m_needs_layout)
			return;
		for(auto &child: m_children)
			child->layout();
		onLayout();
		m_needs_layout = false;
	}
		
	void HudWidget::update(double time_diff) {
		for(auto &child: m_children)
			child->update(time_diff);
		
		animateValue(m_visible_time, time_diff * m_anim_speed, m_is_visible);
		onUpdate(time_diff);

		layout();
	}
	
	void HudWidget::draw() const {
		DASSERT(!m_needs_layout);

		if(isVisible()) {
			onDraw();
	
			float2 offset = rect().min;

			glPushMatrix();
			glTranslatef(offset.x, offset.y, 0.0f);
			for(const auto &child: m_children)
				child->draw();
			glPopMatrix();
		}
	}
		
	void HudWidget::setVisible(bool is_visible, bool animate) {
		m_is_visible = is_visible;
		if(!animate)
			m_visible_time = m_is_visible? 1.0f : 0.0f;
		if(!is_visible && m_parent && m_parent->m_input_focus == this)
			setInputFocus(false);
	}
		
	bool HudWidget::isVisible() const {
		return m_is_visible || m_visible_time > 0.01f;
	}

	bool HudWidget::isShowing() const {
		return m_is_visible && m_visible_time < 1.0f;
	}

	bool HudWidget::isHiding() const {
		return !m_is_visible && m_visible_time > 0.01f;
	}
		
	bool HudWidget::isMouseOver(const io::InputEvent &event) const {
		return isMouseOver(event.mousePos());
	}
	
	bool HudWidget::isMouseOver(const float2 &mouse_pos) const {
		FRect rect = this->rect();

		if(rect.isInside(mouse_pos))
			return true;
		float2 cmouse_pos = mouse_pos - rect.min;
		for(auto &child: m_children)
			if(child->isMouseOver(cmouse_pos))
				return true;
		return false;
	}

}
