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
		normal,
		fire_single,
		fire_burst,
		reload,
		out_of_ammo
	);

	struct WeaponProto: public ProtoImpl<WeaponProto, ItemProto, ProtoId::item_weapon> {
		ItemType::Type itemType() const { return ItemType::weapon; }
		WeaponProto(const TupleParser&);
		void connect();

		string ammo_class_id;
		ProtoRef<ProjectileProto> projectile;
		WeaponClass::Type class_id;
		float damage;
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

		WeaponClass::Type classId() const						{ return proto().class_id; }
		const SoundId soundId(WeaponSoundType::Type type) const	{ return proto().sound_ids[type]; }
		uint attackModes() const								{ return proto().attack_modes; }

		const WeaponProto &proto() const { return static_cast<const WeaponProto&>(*m_proto); }
	};


	
}

#endif
