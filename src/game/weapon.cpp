// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/weapon.h"

#include "game/ammo.h"

namespace game {

	static const EnumMap<WeaponSoundType, const char*> s_suffixes = {{
		"",
		"single",
		"burst",
		"reload",
		"outofammo"
	}};

	static AttackModeFlags parseFlags(const char *string) {
		if(Str(string) == "")
			return {};
		TextParser parser(string);
		EnumFlags<AttackMode> flags;
		parser >> flags;
		return flags;
	}

	WeaponProto::WeaponProto(const TupleParser &parser) :ProtoImpl(parser) {
		ammo_class_id = parser("ammo_class_id");
		impact = parser("impact_id");
		projectile = parser("projectile_id");
		class_id = fromString<WeaponClass>(parser("class_id"));
		damage_mod = parser.get<float>("damage_mod");
		attack_modes = parseFlags(parser("attack_modes"));
		max_ammo = parser.get<int>("max_ammo");
		burst_ammo = parser.get<int>("burst_ammo");

		ranged_range = melee_range = 0.0f;
		accuracy = parser.get<float>("accuracy");

		const char *sound_prefix = parser("sound_prefix");
		for(auto wstype : all<WeaponSoundType>()) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, s_suffixes[wstype]);
			sound_ids[wstype] = SoundId(name);
		}
		if(!sound_ids[WeaponSoundType::normal].isValid())
			sound_ids[WeaponSoundType::normal] = sound_ids[WeaponSoundType::fire_single];
	}

	void WeaponProto::link() {
		projectile.link();
		impact.link();

		auto projectile_modes = AttackMode::single | AttackMode::burst | AttackMode::throwing;

		if(attack_modes & projectile_modes) {
			ASSERT(projectile.isValid());
			ranged_range = projectile->max_range;
		}

		if(attack_modes & ~projectile_modes) {
			ASSERT(impact.isValid());
			melee_range = impact->range;
		}
	}

	float Weapon::range(AttackMode mode) const {
		return isRanged(mode)? proto().ranged_range : proto().melee_range;
	}

	bool Weapon::canKick() const {
		return isOneOf(classId(), WeaponClass::unarmed, WeaponClass::knife, WeaponClass::pistol, WeaponClass::club);
	}
		
	bool Weapon::hasMeleeAttack() const {
		for(uint n = 0; n < count<AttackMode>(); n++) {
			AttackMode mode = (AttackMode)n;
			if(isMelee(mode) && (proto().attack_modes & mode))
				return true;
		}
		return false;
	}

	bool Weapon::hasRangedAttack() const {
		for(uint n = 0; n < count<AttackMode>(); n++) {
			AttackMode mode = (AttackMode)n;
			if(isRanged(mode) && (proto().attack_modes & mode))
				return true;
		}
		return false;
	}

	bool Weapon::canUseAmmo(const Item &item) const {
		return needAmmo() && item.type() == ItemType::ammo && Ammo(item).classId() == proto().ammo_class_id;
	}
		
	float Weapon::estimateDamage(bool include_burst) const {
		//TODO: this function should take some parameters to be mor accurate?

		if(proto().projectile) {
			const ProjectileProto &projectile = *proto().projectile;
			return projectile.impact->damage * proto().damage_mod * max(1, include_burst? proto().burst_ammo : 1);
		}
		if(proto().impact) {
			return proto().impact->damage * proto().damage_mod;
		}

		return 0.0f;
	}
		
	float Weapon::estimateProjectileTime(float distance) const {
		float time = 0.0f;

		if(proto().projectile) {
			const ProjectileProto &projectile = *proto().projectile;
			time += distance / projectile.speed;
		}

		return time;
	}
	
	const string Weapon::paramDesc() const {
		TextFormatter out;
		out.stdFormat("Damage: %.0f", estimateDamage(false));
		if(proto().burst_ammo > 1)
			out(" (x%)", proto().burst_ammo);
		out("\n");
		if(needAmmo())
			out("Max ammo: %\n", maxAmmo());
		if(hasRangedAttack())
			out.stdFormat("Accuracy: %.0f", proto().accuracy);
		return string(out.text());
	}
		

}
