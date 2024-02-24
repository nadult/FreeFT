// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/item.h"

namespace game {

struct AmmoProto : public ProtoImpl<AmmoProto, ItemProto, ProtoId::ammo> {
	ItemType itemType() const { return ItemType::ammo; }
	AmmoProto(const TupleParser &);

	string class_id;
	float damage_mod;
};

struct Ammo : public Item {
  public:
	Ammo(const Item &item) : Item((DASSERT(item.type() == ItemType::ammo), item)) {}
	Ammo(const AmmoProto &proto) : Item(proto) {}
	Ammo() { *this = dummyAmmo(); }

	const string paramDesc() const;

	const string classId() const { return proto().class_id; }
	float damageMod() const { return proto().damage_mod; }
	const AmmoProto &proto() const { return static_cast<const AmmoProto &>(*m_proto); }
};

}
