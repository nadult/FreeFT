/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_BUTTON_H
#define HUD_BUTTON_H

#include "hud/base.h"
#include "game/entity.h"
#include "game/weapon.h"

namespace hud
{

	class HudButton {
	public:
		HudButton(const FRect &target_rect);
		virtual ~HudButton();

		virtual void update(double time_diff);
		virtual void draw() const;
		virtual void setStyle(HudStyle style);

		Color focusColor() const;

		const FRect &targetRect() const { return m_target_rect; }
		const FRect rect() const;

		void setFocus(bool focus) { m_is_focused = focus; }
		bool isFocused() const { return m_is_focused; }
		bool isMouseOver() const;

		void setText(const string &text);

		bool testAccelerator() const;
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
