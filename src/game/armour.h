// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/item.h"


namespace game {

	struct ArmourProto: public ProtoImpl<ArmourProto, ItemProto, ProtoId::armour> {
		ItemType itemType() const { return ItemType::armour; }
		ArmourProto(const TupleParser&);

		ArmourClass class_id;
		float damage_resistance;
		float melee_mod;
		string sound_prefix;
	};

	struct Armour: public Item
	{
	public:
		Armour(const Item &item) :Item((DASSERT(item.type() == ItemType::armour), item)) { }
		Armour(const ArmourProto &proto) :Item(proto) { }
		Armour() { *this = dummy(); }
		
		const string paramDesc() const;

		ArmourClass classId() const		{ return proto().class_id; }

		const ArmourProto &proto() const		{ return static_cast<const ArmourProto&>(*m_proto); }
	};

}
