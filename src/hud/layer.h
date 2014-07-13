/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_LAYER_H
#define HUD_LAYER_H

#include "hud/widget.h"

namespace hud
{
	class HudLayer: public HudWidget {
	public:
		enum { spacing = 5 };

		HudLayer(const FRect &target_rect);
		virtual ~HudLayer();

		const FRect rect() const override;
		void onDraw() const override;

		virtual float backAlpha() const;

	protected:
		bool m_slide_left;
	};

}

#endif
