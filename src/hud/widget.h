/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_WIDGET_H
#define HUD_WIDGET_H

#include "hud/base.h"

namespace hud
{

	class HudWidget: public RefCounter {
	public:
		enum {
			spacing = 15,
			layer_spacing = 5
		};

		HudWidget(const FRect &rect);
		HudWidget(const HudWidget&) = delete;
		void operator=(const HudWidget&) = delete;
		virtual ~HudWidget();
		
		void setStyle(const HudStyle &style);

		void needsLayout();
		void layout();
		void update(double time_diff);
		void draw() const;
		
		bool handleInput(const io::InputEvent&);
		bool handleEvent(const HudEvent&);

		bool handleEvent(HudWidget *widget, HudEvent::Type event_type, int value = 0) {
			return handleEvent(HudEvent(widget, event_type, value));
		}

		bool handleEvent(HudEvent::Type event_type, int value = 0) {
			return handleEvent(nullptr, event_type, value);
		}

		void setRect(const FRect &rect) { m_rect = rect; }
		void setPos(const float2 &pos) { m_rect += pos - m_rect.min; }
		const FRect &targetRect() const { return m_rect; }
		virtual const FRect rect() const { return m_rect; }
		void fitRectToChildren(const float2 &min_size, bool only_visible);

		void setVisible(bool is_visible, bool animate = true);

		bool isVisible() const;
		bool isShowing() const;
		bool isHiding() const;

		void setInputFocus(bool is_focused);

		bool isMouseOver(const io::InputEvent&) const;
		bool isMouseOver(const float2 &mouse_pos) const;

		void attach(Ptr<HudWidget>);
		Ptr<HudWidget> detach(HudWidget*);
		
		const vector<PHudWidget> &children() const { return m_children; }

	protected:
		virtual void onUpdate(double time_diff) { }
		virtual void onLayout() { }
		virtual bool onInput(const io::InputEvent&) { return false; }
		virtual bool onEvent(const HudEvent&) { return false; }
		virtual void onDraw() const { }

		HudWidget *parent() const { return m_parent; }
		
	private:
		HudWidget *m_parent, *m_input_focus;

	protected:
		FRect m_rect;
		HudStyle m_style;
		gfx::PFont m_font, m_big_font;
		vector<Ptr<HudWidget>> m_children;

		float m_anim_speed;
		float m_visible_time;
		float m_mouse_over_time;
		bool m_is_visible;
		bool m_needs_layout;
	};

}

#endif
