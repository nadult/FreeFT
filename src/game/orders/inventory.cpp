/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/inventory.h"
#include "game/actor.h"
#include "game/container.h"

namespace game {

	//TODO: split to separate files

	DropItemOrder::DropItemOrder(Item item, int count)
		:m_item(item), m_count(count) {
	}

	DropItemOrder::DropItemOrder(Stream &sr) :OrderImpl(sr), m_item(sr) {
		sr >> m_count;
	}

	void DropItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_item << m_count;
	}

	bool Actor::handleOrder(DropItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		int item_id = m_inventory.find(order.m_item);
		if(item_id == -1 || order.m_count <= 0)
			return false;

		if(event == ActorEvent::init_order) {
			animate(Action::pickup);
		}
		if(event == ActorEvent::pickup) {
			int count = min(order.m_count, m_inventory[item_id].count);
			m_inventory.remove(item_id, count);
			addNewEntity<ItemEntity>(pos(), order.m_item, count);
		}
		if(event == ActorEvent::anim_finished)
			order.finish();

		return true;
	}



	EquipItemOrder::EquipItemOrder(Item item) :m_item(item) {
	}

	EquipItemOrder::EquipItemOrder(Stream &sr) :OrderImpl(sr), m_item(sr) {
	}

	void EquipItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_item;
	}

	bool Actor::handleOrder(EquipItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		int item_id = m_inventory.find(order.m_item);
		if(item_id == -1)
			return false;
		ItemType::Type item_type = order.m_item.type();

		if(item_type == ItemType::ammo   && m_inventory.ammo().item == order.m_item && m_inventory.ammo().count == m_inventory.weapon().maxAmmo())
			return false;
		if(item_type == ItemType::armour && m_inventory.armour() == order.m_item)
			return false;
		if(item_type == ItemType::weapon && m_inventory.weapon() == order.m_item)
			return false;

		if(event == ActorEvent::init_order) {
			if(item_type == ItemType::weapon) {
				m_inventory.equip(item_id);
				//TODO: maybe we should make sure that animation will be properly updated for a new weapon
				return false;
			}
			if(item_type == ItemType::ammo)
				replicateSound(m_inventory.weapon().soundId(WeaponSoundType::reload), pos());

			//TODO: magic_hi animation when object to be picked up is high enough
			animate(item_type == ItemType::armour? Action::magic_low : Action::magic);
		}
		if(event == ActorEvent::anim_finished) {
			int count = 1;
			if(item_type == ItemType::ammo)
				count = min(m_inventory.weapon().maxAmmo(), m_inventory[item_id].count);
			if(count) {
				m_inventory.equip(item_id, count);
			}
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
		//		replicateSound(m_inventory.weapon().soundId(WeaponSoundType::reload), pos());

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


	TransferItemOrder::TransferItemOrder(EntityRef target, TransferMode mode, Item item, int count)
		:m_target(target), m_mode(mode), m_item(item), m_count(count) {
	}

	TransferItemOrder::TransferItemOrder(Stream &sr) :OrderImpl(sr), m_item(sr) {
		sr >> m_target >> m_mode >> m_count;
	}

	void TransferItemOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_item << m_target << m_mode << m_count;
	}
	
	bool Actor::handleOrder(TransferItemOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(order.m_count <= 0)
			return false;

		if(event == ActorEvent::init_order) {
			Container *container = refEntity<Container>(order.m_target);

			if(container && areAdjacent(*this, *container)) {
				Inventory *src = &m_inventory, *dst = &container->inventory();
				if(order.m_mode == transfer_from)
					swap(src, dst);

				int src_id = src->find(order.m_item);

				if(src_id != -1) {
					int count = min((*src)[src_id].count, order.m_count);
					src->remove(src_id, count);
					dst->add(order.m_item, count);
				}
				container->replicate();
			}
			order.finish();
		}

		return true;
	}

}
