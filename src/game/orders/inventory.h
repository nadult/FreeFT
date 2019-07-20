// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_ORDERS_INVENTORY_H
#define GAME_ORDERS_INVENTORY_H

#include "game/orders.h"
#include "game/inventory.h"

namespace game {

	class DropItemOrder: public OrderImpl<DropItemOrder, OrderTypeId::drop_item> {
	public:
		DropItemOrder(Item item, int count);
		DropItemOrder(Stream&);

		void save(Stream&) const;

		Item m_item;
		int m_count;
	};

	class EquipItemOrder: public OrderImpl<EquipItemOrder, OrderTypeId::equip_item> {
	public:
		EquipItemOrder(Item item);
		EquipItemOrder(Stream&);

		void save(Stream&) const;

		Item m_item;
	};

	class UnequipItemOrder: public OrderImpl<UnequipItemOrder, OrderTypeId::unequip_item> {
	public:
		UnequipItemOrder(ItemType);
		UnequipItemOrder(Stream&);

		void save(Stream&) const;
		
		ItemType m_item_type;
	};

	enum TransferMode: char {
		transfer_to,
		transfer_from,
	};

	class TransferItemOrder: public OrderImpl<TransferItemOrder, OrderTypeId::transfer_item> {
	public:
		TransferItemOrder(EntityRef target, TransferMode mode, Item item, int count);
		TransferItemOrder(Stream&);

		void save(Stream&) const;
		
		EntityRef m_target;
		TransferMode m_mode;
		Item m_item;
		int m_count;
	};

}

#endif
