// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/interact.h"
#include "game/orders/move.h"
#include "game/actor.h"
#include "game/item.h"
#include "game/world.h"

namespace game {

	InteractOrder::InteractOrder(EntityRef target, Maybe<InteractionMode> mode)
		:m_target(target), m_mode(mode), m_is_followup(false) {
	}

	InteractOrder::InteractOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_target >> m_mode >> m_is_followup;
	}

	void InteractOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		sr << m_target << m_mode << m_is_followup;
	}

	bool Actor::handleOrder(InteractOrder &order, EntityEvent event, const EntityEventParams &params) {
		if(event == EntityEvent::init_order) {
			Entity *target = refEntity(order.m_target);
			if(!target)
				return false;

			if(!order.m_mode) {
				if(target->typeId() == EntityId::item)
					order.m_mode = InteractionMode::pickup;
				else
					order.m_mode = InteractionMode::normal;
			}

			IBox my_box(boundingBox());
			IBox other_box = encloseIntegral(target->boundingBox());

			if(areAdjacent(*this, *target)) {
				Action::Type action = order.m_mode == InteractionMode::pickup? Action::pickup :
					other_box.ey() < my_box.ey() * 2 / 3? Action::magic_low : Action::magic;
				animate(action);
				lookAt((float3)other_box.center());
			}
			else {
				if(order.m_is_followup) {
					//printf("Cannot get there!\n");
					return failOrder();
				}
				else {
					int3 target_pos;
				   	if(world()->findClosestPos(target_pos, (int3)pos(), other_box, ref())) {
						order.m_is_followup = true;
						POrder move_order(new MoveOrder(target_pos, true));
						move_order->setFollowup(POrder(order.clone()));
						setOrder(std::move(move_order));
					}
					return false;
				}
			}
		}
		if(event == EntityEvent::pickup && order.m_mode == InteractionMode::pickup) {
			ItemEntity *target = refEntity<ItemEntity>(order.m_target);
			if(target) {
				Item item = target->item();
				int count = target->count();
				//TODO: what will happen if two actors will pickup at the same time?

				target->remove();
				m_inventory.add(item, count);
			}
		}
		if(event == EntityEvent::anim_finished) {
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
