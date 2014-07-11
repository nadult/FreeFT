/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CHARACTER_H
#define HUD_CHARACTER_H

#include "hud/layer.h"
#include "hud/widget.h"
#include "hud/char_icon.h"
#include "hud/edit_box.h"
#include "game/character.h"

namespace hud
{

	class HudCharacter: public HudLayer {
	public:
		enum { spacing = 17 };

		HudCharacter(const FRect &target_rect);
		~HudCharacter();

		void update(bool handle_input, double time_diff) override;
		void draw() const override;

		void setCharacter(const PCharacter&);
		const PCharacter character() const { return m_character; }

	private:
		int m_icon_id;
		vector<pair<ProtoIndex, string>> m_icons;
		Ptr<HudCharIcon> m_icon_box;
		Ptr<HudEditBox> m_name_edit_box;
		PHudWidget m_button_race;
		PHudWidget m_button_up, m_button_down;
		
		PCharacter m_character;
	};

}

#endif
