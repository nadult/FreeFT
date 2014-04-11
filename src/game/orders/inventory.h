/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_INVENTORY_H
#define GAME_ORDERS_INVENTORY_H

#include "game/orders.h"
#include "game/inventory.h"

namespace game {

	class DropItemOrder: public OrderImpl<DropItemOrder, OrderTypeId::drop_item> {
	public:
		DropItemOrder(int inventory_id);
		DropItemOrder(Stream&);

		void save(Stream&) const;

		int m_inventory_id;
	};

	class EquipItemOrder: public OrderImpl<EquipItemOrder, OrderTypeId::equip_item> {
	public:
		EquipItemOrder(int inventory_id);
		EquipItemOrder(Stream&);

		void save(Stream&) const;

		int m_inventory_id;
	};

	class UnequipItemOrder: public OrderImpl<UnequipItemOrder, OrderTypeId::unequip_item> {
	public:
		UnequipItemOrder(ItemType::Type);
		UnequipItemOrder(Stream&);

		void save(Stream&) const;
		
		ItemType::Type m_item_type;
	};

	enum TransferMode: char {
		transfer_to,
		transfer_from,
	};

	class TransferItemOrder: public OrderImpl<TransferItemOrder, OrderTypeId::transfer_item> {
	public:
		TransferItemOrder(EntityRef target, TransferMode mode, int src_inventory_id, int count);
		TransferItemOrder(Stream&);

		void save(Stream&) const;
		
		EntityRef m_target;
		TransferMode m_mode;
		int m_src_inventory_id; //TODO: use Items instead of indexes
		int m_count;
	};

}

#endif
