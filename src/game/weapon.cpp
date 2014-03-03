/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/weapon.h"

namespace game {

	DEFINE_ENUM(WeaponSoundType,
		"single",
		"burst",
		"reload",
		"outofammo"
	);

	WeaponProto::WeaponProto(const TupleParser &parser) :ProtoImpl(parser) {
		ammo_class_id = parser("ammo_class_id");
		projectile = parser("projectile_id");
		class_id = WeaponClass::fromString(parser("class_id"));
		damage = toFloat(parser("damage"));
		attack_modes = AttackModeFlags::fromString(parser("attack_modes"));
		max_ammo = toInt(parser("max_ammo"));
		burst_ammo = toInt(parser("burst_ammo"));

		const char *sound_prefix = parser("sound_prefix");
		for(int n = 0; n < WeaponSoundType::count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, WeaponSoundType::toString(n));
			sound_ids[n] = SoundId(name);
		}
	}
		
	void WeaponProto::connect() {
		projectile.connect();
		if(attack_modes & (AttackModeFlags::single | AttackModeFlags::burst))
			ASSERT(projectile.isValid());
	}

}
