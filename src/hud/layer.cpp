/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/layer.h"
#include "hud/widget.h"
#include "gfx/device.h"
#include "gfx/opengl.h"

using namespace gfx;

namespace hud {

	HudLayer::HudLayer(const FRect &target_rect) :m_target_rect(target_rect), m_is_visible(true), m_visible_time(1.0), m_slide_left(true) {
		m_style = defaultStyle();
	}

	HudLayer::~HudLayer() { }
		
	const FRect HudLayer::rect() const {
		return m_slide_left? 
			m_target_rect - float2((1.0f - m_visible_time) * (m_target_rect.max.x + 5.0f), 0.0f) :
			m_target_rect - float2(0.0f, (1.0f - m_visible_time) * (m_target_rect.max.y + 5.0f));
	}
		
	void HudLayer::setTargetRect(const FRect &rect) {
		m_target_rect = rect;
	}
		
	float HudLayer::backAlpha() const {
		return 0.3f;
	}

	void HudLayer::draw() const {
		if(!isVisible())
			return;

		FRect rect = this->rect();

		glPushMatrix();
		glTranslatef(rect.min.x, rect.min.y, 0.0f);
		rect -= rect.min;

		DTexture::unbind();
		Color color = m_style.layer_color;
		drawQuad(rect, mulAlpha(color, backAlpha()));
		drawBorder(rect, mulAlpha(color, min(backAlpha() * 1.5f, 1.0f)), float2(0, 0), 100.0f);

		for(int n = 0; n < (int)m_widgets.size(); n++)
			m_widgets[n]->draw();

		glPopMatrix();
	}

	void HudLayer::setVisible(bool is_visible, bool animate) {
		m_is_visible = is_visible;
		if(!animate)
			m_visible_time = m_is_visible? 1.0f : 0.0f;
	}

	void HudLayer::setStyle(HudStyle style) {
		m_style = style;
		for(int n = 0; n < (int)m_widgets.size(); n++)
			m_widgets[n]->setStyle(style);
	}
		
	void HudLayer::update(bool is_active, double time_diff) {
		const float2 mouse_pos = float2(getMousePos()) - rect().min;

		float anim_speed = 5.0f;

		m_visible_time += (m_is_visible? 1.0f : -1.0f) * pow(1.0f - m_visible_time + 0.2f, 1.5f) * time_diff * anim_speed;
		m_visible_time = clamp(m_visible_time, 0.0f, 1.0f);

		for(int n = 0; n < (int)m_widgets.size(); n++)
			m_widgets[n]->update(mouse_pos, time_diff);
	}

	bool HudLayer::isMouseOver() const {
		return rect().isInside(getMousePos());
	}
	
	void HudLayer::attach(PHudWidget widget) {
		DASSERT(widget);
		widget->setStyle(m_style);
		m_widgets.emplace_back(std::move(widget));
	}

}
