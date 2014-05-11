/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_HUD_H
#define HUD_HUD_H

#include "game/base.h"
#include "game/entity.h"
#include "hud/layer.h"

namespace hud
{

	class Hud: public HudLayer {
	public:
		Hud(game::PWorld world, game::EntityRef actor_ref);
		~Hud();

		bool isMouseOver() const override;
		void draw() const override;
		void update(bool is_active, double time_diff) override;

		enum LayerId {
			layer_none,
			layer_inventory,
			layer_character,
			layer_options,
		};

	private:
		void sendOrder(game::POrder&&);

		game::PWorld m_world;
		game::EntityRef m_actor_ref;
		gfx::PTexture m_icons;

		PHudWeapon m_hud_weapon;
		PHudCharIcon m_hud_char_icon;
		vector<PHudStance> m_hud_stances;
		vector<PHudWidget> m_hud_buttons;

		LayerId m_selected_layer;
		PHudInventory m_hud_inventory;
		PHudOptions m_hud_options;
	};

}

#endif
