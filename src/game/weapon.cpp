/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/weapon.h"

namespace game {

	DEFINE_ENUM(WeaponSoundType,
		"",
		"single",
		"burst",
		"reload",
		"outofammo"
	);

	WeaponProto::WeaponProto(const TupleParser &parser) :ProtoImpl(parser) {
		ammo_class_id = parser("ammo_class_id");
		impact = parser("impact_id");
		projectile = parser("projectile_id");
		class_id = WeaponClass::fromString(parser("class_id"));
		damage_mod = toFloat(parser("damage_mod"));
		attack_modes = AttackModeFlags::fromString(parser("attack_modes"));
		max_ammo = toInt(parser("max_ammo"));
		burst_ammo = toInt(parser("burst_ammo"));

		ranged_range = melee_range = 0.0f;
		accuracy = toFloat(parser("accuracy"));

		const char *sound_prefix = parser("sound_prefix");
		for(int n = 0; n < WeaponSoundType::count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, WeaponSoundType::toString(n));
			sound_ids[n] = SoundId(name);
		}
		if(!sound_ids[WeaponSoundType::normal].isValid())
			sound_ids[WeaponSoundType::normal] = sound_ids[WeaponSoundType::fire_single];
	}
		
	void WeaponProto::link() {
		projectile.link();
		impact.link();

		int projectile_modes = AttackModeFlags::single | AttackModeFlags::burst | AttackModeFlags::throwing;

		if(attack_modes & projectile_modes) {
			ASSERT(projectile.isValid());
			ranged_range = projectile->max_range;
		}

		if(attack_modes & ~projectile_modes) {
			ASSERT(impact.isValid());
			melee_range = impact->range;
		}
	}
		
	float Weapon::range(AttackMode::Type mode) const {
		return AttackMode::isRanged(mode)? proto().ranged_range : proto().melee_range;
	}

	bool Weapon::canKick() const {
		WeaponClass::Type class_id = classId();
		return	class_id == WeaponClass::unarmed || class_id == WeaponClass::knife ||
				class_id == WeaponClass::pistol || class_id == WeaponClass::club;
	}

}
