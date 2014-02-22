/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_GAME_ARMOUR_H
#define FREEFT_GAME_ARMOUR_H

#include "game/item.h"


namespace game {

	struct ArmourDesc: public ItemDesc, TupleImpl<ArmourDesc> {
		ItemType::Type type() const { return ItemType::armour; }
		ArmourDesc(const TupleParser&);

		ArmourClassId::Type class_id;
		float damage_resistance;
	};

	struct Armour: public Item
	{
	public:
		Armour(const Item &item) :Item((DASSERT(item.type() == ItemType::armour), item)) { }
		Armour(const ArmourDesc &desc) :Item(desc) { }
		Armour(int index) :Item(ArmourDesc::get(index)) { }

		ArmourClassId::Type classId() const { return desc().class_id; }

		const ArmourDesc &desc() const { return static_cast<const ArmourDesc&>(*m_desc); }
	};

}

#endif
