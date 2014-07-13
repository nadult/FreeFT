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
		HudLayer(PWorld world, const FRect &target_rect);
		virtual ~HudLayer();

		const FRect rect() const override;
		void onDraw() const override;

		virtual float backAlpha() const;

		void setPC(game::PPlayableCharacter);
		void sendOrder(game::POrder&&);
		virtual bool canShow() const { return true; }

	protected:
		virtual void onPCSet() { }

		bool m_slide_left;

		game::PWorld m_world;
		game::PPlayableCharacter m_pc;
	};

}

#endif
