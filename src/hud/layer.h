/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_LAYER_H
#define HUD_LAYER_H

#include "hud/base.h"

namespace hud
{
	class HudLayer: public RefCounter {
	public:
		enum { spacing = 5 };

		HudLayer(const FRect &target_rect);
		virtual ~HudLayer();

		void setTargetRect(const FRect &rect);
		const FRect &targetRect() const { return m_target_rect; }

		const FRect rect() const;
		virtual void draw() const;
		virtual void update(bool is_active, double time_diff);
		virtual bool isMouseOver() const;

		bool isVisible() const;
		virtual void setVisible(bool is_visible, bool animate = true);
		
		void setStyle(HudStyle);

	protected:
		void attach(PHudWidget);

		FRect m_target_rect;
		HudStyle m_style;
		vector<PHudWidget> m_widgets;

		float m_visible_time;
		bool m_is_visible;
	};

}

#endif
