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
		Hud(game::PWorld world);
		~Hud();

		bool isMouseOver() const override;
		void draw() const override;

		// Update should always be called before draw in current frame
		void update(bool handle_input, double time_diff) override;
		
		void setVisible(bool is_visible, bool animate = true) override;

		//TODO: remove these?
		void setActor(game::EntityRef);
		void setCharacter(game::PCharacter); //TODO: 

		enum LayerId {
			layer_none,
			layer_inventory,
			layer_character,
			layer_options,
			layer_class,
		};

	private:
		void sendOrder(game::POrder&&);

		game::PWorld m_world;
		game::EntityRef m_actor_ref;
		game::PCharacter m_character;

		PHudWeapon m_hud_weapon;
		PHudCharIcon m_hud_char_icon;
		vector<PHudWidget> m_hud_stances;
		vector<PHudWidget> m_hud_buttons;

		LayerId m_selected_layer;
		Ptr<HudInventory> m_hud_inventory;
		Ptr<HudOptions> m_hud_options;
		Ptr<HudClass> m_hud_class;
		Ptr<HudCharacter> m_hud_character;
	};

}

#endif
