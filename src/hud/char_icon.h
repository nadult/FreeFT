// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef HUD_CHAR_ICON_H
#define HUD_CHAR_ICON_H

#include "hud/button.h"
#include "game/character.h"

namespace hud
{

	class HudCharIcon: public HudButton {
	public:
		HudCharIcon(const FRect &target_rect);
		~HudCharIcon();

		void setHP(int current, int max) { m_current_hp = current; m_max_hp = max; }
		void setCharacter(PCharacter character) {  m_character = character; }
		
		void onDraw(Renderer2D&) const override;

	private:
		PCharacter m_character;
		int m_current_hp, m_max_hp;
	};

}

#endif
