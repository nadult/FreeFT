/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_WEAPON_H
#define GAME_WEAPON_H

#include "game/item.h"
#include "game/projectile.h"

namespace game {

	//TODO: melee sounds

	DEFINE_ENUM(WeaponSoundType,
		normal,
		fire_single,
		fire_burst,
		reload,
		out_of_ammo
	);

	struct WeaponProto: public ProtoImpl<WeaponProto, ItemProto, ProtoId::weapon> {
		ItemType itemType() const { return ItemType::weapon; }
		WeaponProto(const TupleParser&);
		void link();

		string ammo_class_id;

		float melee_range;
		float ranged_range;
		float accuracy;

		ProtoRef<ProjectileProto> projectile;
		ProtoRef<ImpactProto> impact;

		WeaponClass class_id;
		float damage_mod;
		unsigned attack_modes;
		int max_ammo, burst_ammo;

		EnumMap<WeaponSoundType, SoundId> sound_ids;
	};

	struct Weapon: public Item
	{
	public:
		Weapon(const WeaponProto &proto) :Item(proto) { }
		Weapon(const Item &item) :Item((DASSERT(item.type() == ItemType::weapon), item)) { }
		Weapon() { *this = dummyWeapon(); }
		
		const string paramDesc() const;
		
		const ProjectileProto *projectileProto() const			{ return proto().projectile; }

		float range(AttackMode mode) const;
		bool canKick() const;

		bool hasMeleeAttack() const;
		bool hasRangedAttack() const;
		float estimateDamage(bool include_burst = true) const;
		float estimateProjectileTime(float distance) const;

		WeaponClass classId() const						{ return proto().class_id; }
		const SoundId soundId(WeaponSoundType type) const	{ return proto().sound_ids[type]; }
		unsigned attackModes() const							{ return proto().attack_modes; }

		int maxAmmo() const { return proto().max_ammo; }
		bool needAmmo() const { return !proto().ammo_class_id.empty(); }
		bool canUseAmmo(const Item &item) const;

		const WeaponProto &proto() const { return static_cast<const WeaponProto&>(*m_proto); }
	};


	
}

#endif
