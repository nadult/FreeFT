/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_HUD_H
#define HUD_HUD_H

#include "game/base.h"
#include "game/entity.h"
#include "hud/base.h"

namespace hud
{

	class Hud: public RefCounter {
	public:
		Hud(game::PWorld world, game::EntityRef actor_ref);
		~Hud();

		bool isMouseOver() const;
		void draw() const;
		void update(bool is_active, double time_diff);
		void setStyle(HudStyle);

	private:
		void sendOrder(game::POrder&&);

		HudStyle m_style;
		game::PWorld m_world;
		game::EntityRef m_actor_ref;

		gfx::PTexture m_icons;
		vector<HudButton*> m_all_buttons;

		PHudWeapon m_hud_weapon;
		PHudCharIcon m_hud_char_icon;
		vector<PHudStance> m_hud_stances;
		vector<PHudButton> m_hud_buttons;

		FRect m_back_rect;
	};

}

#endif
