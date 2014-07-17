/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CLASS_H
#define HUD_CLASS_H

#include "hud/layer.h"
#include "hud/button.h"

namespace hud
{

	class HudClassButton: public HudRadioButton {
	public:
		enum { spacing = 17 };

		HudClassButton(const FRect &rect);

		void onDraw() const override;
		Color backgroundColor() const override;
	};

	class HudClass: public HudLayer {
	public:
		HudClass(const FRect &target_rect);
		~HudClass();

		bool canShow() const override;

	private:
		void onUpdate(double time_diff) override;
		bool onEvent(const HudEvent&) override;
		void onLayout() override;

		int m_offset, m_class_count;

		vector<Ptr<HudClassButton>> m_buttons;
		PHudButton m_button_up, m_button_down;
	};

}

#endif
