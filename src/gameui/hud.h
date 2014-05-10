/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAMEUI_HUD_H
#define GAMEUI_HUD_H

#include "game/base.h"
#include "game/entity.h"
#include "game/weapon.h"

namespace ui
{

	class HUDButton {
	public:
		HUDButton(const FRect &target_rect, float max_offset);
		virtual ~HUDButton() = default;

		virtual void update(double time_diff);
		virtual void draw() const;
		bool isMouseOver() const;

		const FRect &targetRect() const { return m_target_rect; }
		const FRect rect() const;

	private:
		FRect m_target_rect;
		float m_max_offset;
	};

	typedef unique_ptr<HUDButton> PHUDButton;

	class HUDCharacter: public HUDButton {
	public:
		HUDCharacter(const FRect &target_rect);

		void setHP(int current, int max) { m_current_hp = current; m_max_hp = max; }
		void setCharacter(game::PCharacter character) {  m_character = character; }
		
		void update(double time_diff) override;
		void draw() const override;

	private:
		game::PCharacter m_character;
		int m_current_hp, m_max_hp;
	};

	class HUDWeapon: public HUDButton {
	public:
		HUDWeapon(const FRect &target_rect);

		void setWeapon(game::Weapon weapon) { m_weapon = weapon; }
		void setAmmoCount(int count) { m_ammo_count = count; }
		void setAttackMode(game::AttackMode::Type mode) { m_attack_mode = mode; }

		void update(double time_diff) override;
		void draw() const override;

	private:
		game::Weapon m_weapon;
		game::AttackMode::Type m_attack_mode;
		int m_ammo_count;
	};

	class HUD: public RefCounter {
	public:
		HUD(game::PWorld world, game::EntityRef actor_ref);
		~HUD();

		bool isMouseOver() const;
		void draw() const;
		void update(double time_diff);

	private:
		game::PWorld m_world;
		game::EntityRef m_actor_ref;

		unique_ptr<HUDWeapon> m_hud_weapon;
		unique_ptr<HUDCharacter> m_hud_character;
		vector<PHUDButton> m_hud_stances;
		vector<PHUDButton> m_hud_options;
	};

}

#endif
