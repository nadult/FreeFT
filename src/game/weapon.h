/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_GAME_WEAPON_H
#define FREEFT_GAME_WEAPON_H

#include "game/item.h"
#include "game/projectile.h"

namespace game {

	//TODO: melee sounds

	DECLARE_ENUM(WeaponSoundType,
		fire_single,
		fire_burst,
		reload,
		out_of_ammo
	);

	struct WeaponDesc: public ItemDesc, TupleImpl<WeaponDesc> {
		ItemType::Type type() const { return ItemType::weapon; }
		WeaponDesc(const TupleParser&);
		void connect();

		string ammo_class_id;
		TupleRef<ProjectileDesc> projectile_ref;
		WeaponClassId::Type class_id;
		float damage;
		uint attack_modes;
		int max_ammo, burst_ammo;

		SoundId sound_ids[WeaponSoundType::count];
	};

	struct Weapon: public Item
	{
	public:
		Weapon(const WeaponDesc &desc) :Item(desc) { }
		Weapon(const Item &item) :Item((DASSERT(item.type() == ItemType::weapon), item)) { }
		Weapon(int index) :Weapon(WeaponDesc::get(index)) { }

		const ProjectileDesc *projectileDesc() const			{ return (const ProjectileDesc*)desc().projectile_ref; }
		WeaponClassId::Type classId() const						{ return desc().class_id; }
		const SoundId soundId(WeaponSoundType::Type type) const	{ return desc().sound_ids[type]; }
		uint attackModes() const								{ return desc().attack_modes; }

		const WeaponDesc &desc() const { return static_cast<const WeaponDesc&>(*m_desc); }
	};


	
}

#endif
