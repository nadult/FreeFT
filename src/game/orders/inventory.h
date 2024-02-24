// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/inventory.h"
#include "game/orders.h"

namespace game {

class DropItemOrder : public OrderImpl<DropItemOrder, OrderTypeId::drop_item> {
  public:
	DropItemOrder(Item item, int count);
	DropItemOrder(MemoryStream &);

	void save(MemoryStream &) const;

	Item m_item;
	int m_count;
};

class EquipItemOrder : public OrderImpl<EquipItemOrder, OrderTypeId::equip_item> {
  public:
	EquipItemOrder(Item item);
	EquipItemOrder(MemoryStream &);

	void save(MemoryStream &) const;

	Item m_item;
};

class UnequipItemOrder : public OrderImpl<UnequipItemOrder, OrderTypeId::unequip_item> {
  public:
	UnequipItemOrder(ItemType);
	UnequipItemOrder(MemoryStream &);

	void save(MemoryStream &) const;

	ItemType m_item_type;
};

enum TransferMode : char {
	transfer_to,
	transfer_from,
};

class TransferItemOrder : public OrderImpl<TransferItemOrder, OrderTypeId::transfer_item> {
  public:
	TransferItemOrder(EntityRef target, TransferMode mode, Item item, int count);
	TransferItemOrder(MemoryStream &);

	void save(MemoryStream &) const;

	EntityRef m_target;
	TransferMode m_mode;
	Item m_item;
	int m_count;
};

}
