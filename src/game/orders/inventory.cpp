/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/inventory.h"
#include "game/actor.h"
#include "game/container.h"
#include "game/world.h"

namespace game {

	//TODO: split to separate files

	DropItemOrder::DropItemOrder(int inventory_id) :m_inventory_id(inventory_id) {
	}

	DropItemOrder::DropItemOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_inventory_id;
	}

	void DropItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_inventory_id;
	}

	void Actor::handleOrder(DropItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		int item_id = order.m_inventory_id;

		if(event == ActorEvent::init_order) {
			if(m_inventory.isValidId(item_id))
				animate(ActionId::pickup);
			else
				order.finish();
		}
		if(event == ActorEvent::pickup) {
			if(m_inventory.isValidId(item_id)) {
				Item item = m_inventory[item_id].item;
				int count = m_inventory[item_id].count;
				if(item.type() != ItemType::ammo)
					count = 1;
				m_inventory.remove(item_id, count);
				addEntity(new ItemEntity(item, count, pos())); 
			}
		}
		if(event == ActorEvent::anim_finished)
			order.finish();
	}



	EquipItemOrder::EquipItemOrder(int inventory_id) :m_inventory_id(inventory_id) {
	}

	EquipItemOrder::EquipItemOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_inventory_id;
	}

	void EquipItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_inventory_id;
	}

	void Actor::handleOrder(EquipItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		//TODO: magic_hi animation when object to be picked up is high enough
		
		if(event == ActorEvent::init_order)
			animate(ActionId::magic1);
		if(event == ActorEvent::anim_finished) {
			ItemType::Type changed_item = ItemType::invalid;

			int item_id = order.m_inventory_id;

			if(m_inventory.isValidId(item_id) && canEquipItem(item_id)) {
				//TODO: reloading ammo
				changed_item = m_inventory[item_id].item.type();
				if(!m_inventory.equip(item_id))
					changed_item = ItemType::invalid;
			}
			
			if(changed_item == ItemType::armour)
				updateArmour();
			order.finish();
		}

	}



	UnequipItemOrder::UnequipItemOrder(ItemType::Type item_type) :m_item_type(item_type) {

	}

	UnequipItemOrder::UnequipItemOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_item_type;
	}

	void UnequipItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_item_type;
	}

	void Actor::handleOrder(UnequipItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order)
			animate(ActionId::magic1);
		if(event == ActorEvent::anim_finished) {
			ItemType::Type changed_item = ItemType::invalid;

			ItemType::Type type = order.m_item_type;
			if(m_inventory.unequip(type) != -1)
				changed_item = type;
				
			if(changed_item == ItemType::armour)
				updateArmour();
			order.finish();
		}
	}
	


	TransferItemOrder::TransferItemOrder(EntityRef target, TransferMode mode, int src_inventory_id, int count)
		:m_target(target), m_mode(mode), m_src_inventory_id(src_inventory_id), m_count(count) {
	}

	TransferItemOrder::TransferItemOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_target >> m_mode >> m_src_inventory_id >> m_count;
	}

	void TransferItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_target << m_mode << m_src_inventory_id << m_count;
	}
	
	void Actor::handleOrder(TransferItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			Container *container = refEntity<Container>(order.m_target);

			if(container && areAdjacent(*this, *container)) {
				Inventory *src = &m_inventory, *dst = &container->inventory();
				if(order.m_mode == transfer_from)
					swap(src, dst);
				if(src->isValidId(order.m_src_inventory_id) && (*src)[order.m_src_inventory_id].count >= order.m_count) {
					Item item = (*src)[order.m_src_inventory_id].item;
					src->remove(order.m_src_inventory_id, order.m_count);
					dst->add(item, order.m_count);
				}
			}
			order.finish();
		}
	}

}
