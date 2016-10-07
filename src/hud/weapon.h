/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_WEAPON_H
#define HUD_WEAPON_H

#include "game/base.h"
#include "game/entity.h"
#include "game/weapon.h"
#include "hud/button.h"

namespace hud
{

	class HudWeapon: public HudButton {
	public:
		HudWeapon(const FRect &target_rect);

		void setWeapon(Weapon weapon) { m_weapon = weapon; }
		void setAmmoCount(int count) { m_ammo_count = count; }
		void setAttackMode(AttackMode mode) { m_attack_mode = mode; }

		void onDraw(Renderer2D&) const override;

	private:
		Weapon m_weapon;
		Maybe<AttackMode> m_attack_mode;
		int m_ammo_count = 0;
	};

}

#endif
