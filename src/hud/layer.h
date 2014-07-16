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
		enum SlideMode {
			slide_left,
			slide_top,
		};

		HudLayer(const FRect &target_rect, SlideMode slide_mode = slide_left);
		virtual ~HudLayer();

		const FRect rect() const override;

		virtual float backAlpha() const;

		void setPCController(game::PPCController);
		virtual bool canShow() const { return true; }

	protected:
		void onDraw() const override;
		
		game::PPCController m_pc_controller;

	private:
		SlideMode m_slide_mode;
	};

}

#endif
