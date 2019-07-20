// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/layer.h"

namespace hud
{

	class HudMainPanel: public HudLayer {
	public:
		HudMainPanel(const FRect &rect);
		~HudMainPanel();
	
		void setCurrentLayer(int layer_id);
		void setCanShowLayer(int layer_id, bool can_show);

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
