/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_WIDGET_H
#define HUD_WIDGET_H

#include "hud/base.h"
#include "game/entity.h"
#include "game/weapon.h"

namespace hud
{

	class HudWidget: public RefCounter {
	public:
		HudWidget(const FRect &target_rect);
		virtual ~HudWidget();

		virtual void update(const float2 &mouse_pos, double time_diff);
		virtual void draw() const;
		virtual void setStyle(HudStyle style);

		Color focusColor() const;

		const FRect &targetRect() const { return m_target_rect; }
		const FRect rect() const;

		void setFocus(bool focus) { m_is_focused = focus; }
		bool isFocused() const { return m_is_focused; }

		bool isMouseOver(const float2 &mouse_pos) const;
		bool isPressed(const float2 &mouse_pos) const;

		void setText(const string &text);

		void setAccelerator(int key) { m_accelerator = key; }
		int accelerator() const { return m_accelerator; }

	protected:
		HudStyle m_style;
		gfx::PFont m_font;

		string m_text;
		FRect m_target_rect;
		float m_over_time;
		float m_focus_time;

		int m_accelerator;
		bool m_is_focused;
	};

}

#endif
