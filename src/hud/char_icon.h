/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CHAR_ICON_H
#define HUD_CHAR_ICON_H

#include "hud/widget.h"

namespace hud
{

	class HudCharIcon: public HudWidget {
	public:
		HudCharIcon(const FRect &target_rect);
		~HudCharIcon();

		void setHP(int current, int max) { m_current_hp = current; m_max_hp = max; }
		void setCharacter(PCharacter character) {  m_character = character; }
		
		void draw() const override;

	private:
		PCharacter m_character;
		int m_current_hp, m_max_hp;
	};

}

#endif
