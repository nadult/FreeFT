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
		HudMainPanel(const FRect &rect);
		~HudMainPanel();
	
		void setLayerId(int layer_id);

	private:
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;

		//TODO: change attack mode on right click
		PHudWeapon m_hud_weapon;
		PHudCharIcon m_hud_char_icon;
		vector<PHudButton> m_hud_stances;
		vector<PHudButton> m_hud_buttons;
	};

}

#endif
