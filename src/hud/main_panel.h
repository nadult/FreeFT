/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_MAIN_PANEL_H
#define HUD_MAIN_PANEL_H

#include "hud/layer.h"

namespace hud
{

	class HudMainPanel: public HudLayer {
	public:
		HudMainPanel(game::PWorld, const FRect &rect);
		~HudMainPanel();

		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;

	private:
		PHudWeapon m_hud_weapon;
		PHudCharIcon m_hud_char_icon;
		vector<PHudButton> m_hud_stances;
		vector<PHudButton> m_hud_buttons;
	};

}

#endif
