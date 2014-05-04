/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_WEAPON_H
#define GAME_WEAPON_H

#include "game/item.h"
#include "game/projectile.h"

namespace game {

	//TODO: melee sounds

	DECLARE_ENUM(WeaponSoundType,
		normal,
		fire_single,
		fire_burst,
		reload,
		out_of_ammo
	);

	struct WeaponProto: public ProtoImpl<WeaponProto, ItemProto, ProtoId::item_weapon> {
		ItemType::Type itemType() const { return ItemType::weapon; }
		WeaponProto(const TupleParser&);
		void link();

		string ammo_class_id;

		float melee_range;
		float ranged_range;
		float accuracy;

		ProtoRef<ProjectileProto> projectile;
		ProtoRef<ImpactProto> impact;

		WeaponClass::Type class_id;
		float damage_mod;
		uint attack_modes;
		int max_ammo, burst_ammo;

		SoundId sound_ids[WeaponSoundType::count];
	};

	struct Weapon: public Item
	{
	public:
		Weapon(const WeaponProto &proto) :Item(proto) { }
		Weapon(const Item &item) :Item((DASSERT(item.type() == ItemType::weapon), item)) { }

		const ProjectileProto *projectileProto() const			{ return proto().projectile; }

		float range(AttackMode::Type mode) const;
		bool canKick() const;

		bool hasMeleeAttack() const;
		bool hasRangedAttack() const;
		float estimateDamage() const;
		float estimateProjectileTime(float distance) const;

		WeaponClass::Type classId() const						{ return proto().class_id; }
		const SoundId soundId(WeaponSoundType::Type type) const	{ return proto().sound_ids[type]; }
		uint attackModes() const								{ return proto().attack_modes; }

		const WeaponProto &proto() const { return static_cast<const WeaponProto&>(*m_proto); }
	};


	
}

#endif
