/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_AMMO_H
#define GAME_AMMO_H

#include "game/item.h"


namespace game {

	struct AmmoProto: public ProtoImpl<AmmoProto, ItemProto, ProtoId::item_ammo> {
		ItemType::Type itemType() const { return ItemType::ammo; }
		AmmoProto(const TupleParser&);

		string class_id;
		float damage_mod;
	};

	struct Ammo: public Item
	{
	public:
		Ammo(const Item &item) :Item((DASSERT(item.type() == ItemType::ammo), item)) { }
		Ammo(const AmmoProto &proto) :Item(proto) { }
		Ammo() { *this = dummyAmmo(); }

		const string classId() const		{ return proto().class_id; }
		float damageMod() const				{ return proto().damage_mod; }
		const AmmoProto &proto() const		{ return static_cast<const AmmoProto&>(*m_proto); }
	};

}

#endif
