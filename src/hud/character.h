/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CHARACTER_H
#define HUD_CHARACTER_H

#include "hud/layer.h"
#include "hud/button.h"
#include "hud/char_icon.h"
#include "hud/edit_box.h"
#include "game/character.h"

namespace hud
{

	class HudCharacter: public HudLayer {
	public:
		enum { spacing = 17 };

		HudCharacter(PWorld world, const FRect &target_rect);
		~HudCharacter();

		void onUpdate(double time_diff) override;

	private:
		int m_icon_id;
		vector<pair<ProtoIndex, string>> m_icons;
		Ptr<HudCharIcon> m_icon_box;
		Ptr<HudEditBox> m_name_edit_box;
		PHudButton m_button_race;
		PHudButton m_button_up, m_button_down;
	};

}

#endif
