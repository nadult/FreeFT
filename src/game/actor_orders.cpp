#include "game/actor.h"
#include "game/container.h"
#include "game/world.h"
#include <cstdio>

namespace game {

	Order moveOrder(int3 target_pos, bool run)	{
		Order new_order(OrderId::move);
		new_order.move = Order::Move{target_pos, run};
		return new_order;
	}
	Order doNothingOrder() {
		Order new_order(OrderId::do_nothing);
		return new_order;
	}
	Order changeStanceOrder(int next_stance) {
		Order new_order(OrderId::change_stance);
		new_order.change_stance = Order::ChangeStance{next_stance};
		return new_order;
	}
	Order attackOrder(int attack_mode, const int3 &target_pos) {
		Order new_order(OrderId::attack);
		new_order.attack = Order::Attack{target_pos, attack_mode};
		return new_order;
	}
	Order interactOrder(Entity *target, InteractionMode mode) {
		Order new_order(OrderId::interact);
		DASSERT(target);
		new_order.target = target;
		new_order.interact = Order::Interact{mode, false};
		return new_order;
	}
	Order dropItemOrder(int item_id) {
		Order new_order(OrderId::drop_item);
		new_order.drop_item = Order::DropItem{item_id};
		return new_order;
	}
	Order equipItemOrder(int item_id) {
		Order new_order(OrderId::equip_item);
		new_order.equip_item = Order::EquipItem{item_id};
		return new_order;
	}
	Order unequipItemOrder(InventorySlotId::Type slot_id) {
		Order new_order(OrderId::unequip_item);
		DASSERT(slot_id >= 0 && slot_id < InventorySlotId::count);
		new_order.unequip_item = Order::UnequipItem{slot_id};
		return new_order;
	}
	Order transferItemOrder(Entity *target, TransferMode mode, int item_id, int count) {
		Order new_order(OrderId::transfer_item);
		DASSERT(target); //TODO: entity pointer may become invalid
		new_order.target = target;
		new_order.transfer_item = Order::TransferItem{item_id, count, mode};
		return new_order;
	}

	void Actor::setNextOrder(const Order &order) {
		m_next_order = order;
	}

	void Actor::issueNextOrder() {
		if(m_order.id == OrderId::do_nothing && m_next_order.id == OrderId::do_nothing)
			return;
		
		if(m_order.id == OrderId::change_stance) {
			m_stance_id = (StanceId::Type)(m_stance_id - m_order.change_stance.next_stance);
			//TODO: different bboxes for stances
			//TODO: support for non-quadratic actor bboxes
	//		m_bbox = m_sprite->m_bbox;
	//		if(m_stance_id == StanceId::crouching && m_bbox.y == 9)
	//			m_bbox.y = 5;
	//		if(m_stance_id == StanceId::prone && m_bbox.y == 9)
	//			m_bbox.y = 2;
			DASSERT(m_stance_id >= 0 && m_stance_id < StanceId::count);
		}
		
		if(m_next_order.id == OrderId::move) {
			issueMoveOrder();
			m_next_order = doNothingOrder();
		}
		else if(m_next_order.id == OrderId::interact) {
			IBox my_box(boundingBox());
			IBox other_box = enclosingIBox(m_next_order.target->boundingBox());

			if(areAdjacent(*this, *m_next_order.target)) {
				m_order = m_next_order;
				m_next_order = doNothingOrder();
				ActionId::Type action = m_order.interact.mode == interact_pickup? ActionId::pickup :
					other_box.max.y < my_box.max.y * 2 / 3? ActionId::magic2 : ActionId::magic1;
				setSequence(action);
				lookAt(other_box.center());
			}
			else {
				if(m_next_order.interact.waiting_for_move) {
					printf("Cannot get there!\n");
					m_order = m_next_order = doNothingOrder();
				}
				else {
					int3 target_pos = my_box.min;
					if(my_box.max.x < other_box.min.x)
						target_pos.x = other_box.min.x - my_box.width();
					else if(my_box.min.x > other_box.max.x)
						target_pos.x = other_box.max.x;
					if(my_box.max.z < other_box.min.z)
						target_pos.z = other_box.min.z - my_box.depth();
					else if(my_box.min.z > other_box.max.z)
						target_pos.z = other_box.max.z;
					
					target_pos = m_world->naviMap().findClosestCorrectPos(target_pos, other_box);
					Order order = m_next_order;
					order.interact.waiting_for_move = true;
					m_next_order = moveOrder(target_pos, true);
					issueMoveOrder();
					if(m_order.id == OrderId::move)
						m_next_order = order;
				}
			}
		}
		else {
			if(m_next_order.id == OrderId::change_stance) {
				int next_stance = m_next_order.change_stance.next_stance;

				if(next_stance > 0 && m_stance_id != StanceId::standing)
					setSequence(ActionId::stance_up);
				else if(next_stance < 0 && m_stance_id != StanceId::prone)
					setSequence(ActionId::stance_down);
				else
					m_next_order = doNothingOrder();
			}
			else if(m_next_order.id == OrderId::attack) {
				roundPos();
				lookAt(m_next_order.attack.target_pos);
				setSequence(ActionId::attack1);
			}
			else if(m_next_order.id == OrderId::drop_item) {
				int item_id = m_next_order.drop_item.item_id;
				if(m_inventory.isValidId(item_id))
					setSequence(ActionId::pickup);
				else
					m_next_order = doNothingOrder();
			}
			else if(m_next_order.id == OrderId::transfer_item) {
				auto params = m_next_order.transfer_item;
				const EntityRef &target = m_next_order.target;

				if(areAdjacent(*this, *target) && target->entityType() == entity_container) {
					Container *container = static_cast<Container*>(target.get());
					Inventory *src = &m_inventory, *dst = &container->inventory();
					if(params.mode == transfer_from)
						swap(src, dst);
					if(src->isValidId(params.item_id) && (*src)[params.item_id].count >= params.count) {
						Item item = (*src)[params.item_id].item;
						src->remove(params.item_id, params.count);
						dst->add(item, params.count);
					}
				}
				m_next_order = doNothingOrder();
			}
			else if(m_next_order.id == OrderId::equip_item || m_next_order.id == OrderId::unequip_item) {
				InventorySlotId::Type changed_slot = InventorySlotId::invalid;

				if(m_next_order.id == OrderId::equip_item) {
					int item_id = m_next_order.equip_item.item_id;
					if(item_id >= 0 && item_id < m_inventory.size() && canEquipItem(item_id))
						changed_slot = m_inventory.equip(item_id);
				}
				else {
					InventorySlotId::Type slot_id = m_next_order.unequip_item.slot_id;
					if(m_inventory.unequip(slot_id) != -1)
						changed_slot = slot_id;
				}

				m_next_order = doNothingOrder();

				if(changed_slot == InventorySlotId::armour)
					updateArmour();
				else if(changed_slot == InventorySlotId::weapon)
					updateWeapon();
			}


			m_order = m_next_order;
			m_next_order = doNothingOrder();
		}
		
		if(m_order.id == OrderId::do_nothing)	
			setSequence(ActionId::standing);

		m_issue_next_order = false;
	}

	void Actor::issueMoveOrder() {
		OrderId::Type order_id = m_next_order.id;
		int3 new_pos = m_next_order.move.target_pos;
		DASSERT(order_id == OrderId::move);

		new_pos = max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents

		int3 cur_pos = (int3)pos();
		vector<int3> tmp_path = m_world->findPath(cur_pos, new_pos);

		if(cur_pos == new_pos || tmp_path.empty()) {
			m_order = doNothingOrder();
			return;
		}

		m_last_pos = cur_pos;

		m_path.clear();
		m_path_t = 0;
		m_path_pos = 0;
		roundPos();

		m_order = m_next_order;

		for(int n = 1; n < (int)tmp_path.size(); n++) {
			cur_pos = tmp_path[n - 1];
			new_pos = tmp_path[n];
			if(new_pos == cur_pos)
				continue;

			MoveVector mvec(tmp_path[n - 1].xz(), tmp_path[n].xz());

			while(mvec.dx) {
				int step = min(mvec.dx, 3);
				m_path.push_back(cur_pos += int3(mvec.vec.x * step, 0, 0));
				mvec.dx -= step;
			}
			while(mvec.dy) {
				int step = min(mvec.dy, 3);
				m_path.push_back(cur_pos += int3(0, 0, mvec.vec.y * step));
				mvec.dy -= step;
			}
			while(mvec.ddiag) {
				int dstep = min(mvec.ddiag, 3);
				m_path.push_back(cur_pos += asXZ(mvec.vec) * dstep);
				mvec.ddiag -= dstep;
			}
		}
			
		if(m_path.size() <= 1 || m_stance_id != StanceId::standing)
			m_order.move.run = 0;
		setSequence(m_order.move.run? ActionId::running : ActionId::walking);

		DASSERT(!m_path.empty());
	}



}
