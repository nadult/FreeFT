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

	bool Actor::handleOrder(DropItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		int item_id = order.m_inventory_id;

		if(event == ActorEvent::init_order) {
			if(m_inventory.isValidId(item_id))
				animate(Action::pickup);
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
				addNewEntity<ItemEntity>(pos(), item, count);
			}
		}
		if(event == ActorEvent::anim_finished)
			order.finish();

		return true;
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

	bool Actor::handleOrder(EquipItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(!m_inventory.isValidId(order.m_inventory_id) || !canEquipItem(order.m_inventory_id))
			return false;
		const Item &item = m_inventory[order.m_inventory_id].item;
		ItemType::Type item_type = item.type(); //TODO: type->typeId

		//TODO: magic_hi animation when object to be picked up is high enough
		
		if(event == ActorEvent::init_order) {
			if(item_type == ItemType::weapon) {
				m_inventory.equip(order.m_inventory_id);
				return false;
			}

			if(item_type == ItemType::ammo)
				world()->playSound(m_inventory.weapon().soundId(WeaponSoundType::reload), pos());

			animate(item_type == ItemType::armour? Action::magic_low : Action::magic);
		}
		if(event == ActorEvent::anim_finished) {
			int count = 1;
			if(item_type == ItemType::ammo)
				count = min(m_inventory.weapon().proto().max_ammo, m_inventory[order.m_inventory_id].count);
			if(count)
				m_inventory.equip(order.m_inventory_id, count);
			if(item_type == ItemType::armour)
				updateArmour();
			return false;
		}

		return true;
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

	bool Actor::handleOrder(UnequipItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			if(order.m_item_type == ItemType::weapon) {
				m_inventory.unequip(order.m_item_type);
				return false;
			}
		// TODO: play only half of the sound and then blend out?
		//	if(order.m_item_type == ItemType::ammo)
		//		world()->playSound(m_inventory.weapon().soundId(WeaponSoundType::reload), pos());

			animate(order.m_item_type == ItemType::armour? Action::magic_low : Action::magic);
		}
		if(event == ActorEvent::anim_finished) {
			if(m_inventory.unequip(order.m_item_type) != -1)
				if(order.m_item_type == ItemType::armour)
					updateArmour();
			return false;
		}

		return true;
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
	
	bool Actor::handleOrder(TransferItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			Container *container = refEntity<Container>(order.m_target);

			if(container && areAdjacent(*this, *container) && !isClient()) {
				Inventory *src = &m_inventory, *dst = &container->inventory();
				if(order.m_mode == transfer_from)
					swap(src, dst);
				if(src->isValidId(order.m_src_inventory_id) && (*src)[order.m_src_inventory_id].count >= order.m_count) {
					Item item = (*src)[order.m_src_inventory_id].item;
					src->remove(order.m_src_inventory_id, order.m_count);
					dst->add(item, order.m_count);
				}
				container->replicate();
			}
			order.finish();
		}

		return true;
	}

}
