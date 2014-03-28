/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/inventory.h"
#include "game/actor.h"
#include "game/item.h"
#include "game/world.h"

namespace game {

	DEFINE_ENUM(InteractionMode,
		"normal",
		"pickup",
		"use_item"
	);

	InteractOrder::InteractOrder(EntityRef target, InteractionMode::Type mode)
		:m_target(target), m_mode(mode), m_is_followup(false) {
	}

	InteractOrder::InteractOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_target >> m_mode >> m_is_followup;
	}

	void InteractOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_target << m_mode << m_is_followup;
	}

	bool Actor::handleOrder(InteractOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			Entity *target = refEntity(order.m_target);
			if(!target)
				return false;

			if(order.m_mode == InteractionMode::undefined) {
				if(target->typeId() == EntityId::item)
					order.m_mode = InteractionMode::pickup;
				else
					order.m_mode = InteractionMode::normal;
			}

			IBox my_box(boundingBox());
			IBox other_box = enclosingIBox(target->boundingBox());

			if(areAdjacent(*this, *target)) {
				Action::Type action = order.m_mode == InteractionMode::pickup? Action::pickup :
					other_box.max.y < my_box.max.y * 2 / 3? Action::magic_low : Action::magic;
				animate(action);
				lookAt(other_box.center());
			}
			else {
				if(order.m_is_followup) {
					printf("Cannot get there!\n");
					order.finish();
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
					
					target_pos = world()->naviMap().findClosestCorrectPos(target_pos, other_box);

					order.m_is_followup = true;
					POrder move_order = new MoveOrder(target_pos, true);
					move_order->setFollowup(order.clone());
					setOrder(std::move(move_order));
					order.finish();
				}
			}
		}
		if(event == ActorEvent::pickup && order.m_mode == InteractionMode::pickup) {
			ItemEntity *target = refEntity<ItemEntity>(order.m_target);
			if(target) {
				Item item = target->item();
				int count = target->count();
				//TODO: what will happen if two actors will pickup at the same time?

				target->remove();
				m_inventory.add(item, count);
			}
		}
		if(event == ActorEvent::anim_finished) {
			if(order.m_mode == InteractionMode::normal) {
				Entity *target = refEntity(order.m_target);
				if(target)
					target->interact(this);
			}

			order.finish();
		}

		return true;
	}

}
