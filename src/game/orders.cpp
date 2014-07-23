/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor.h"
#include "game/orders.h"
#include "game/container.h"
#include "game/world.h"
#include "game/actor_ai.h"
#include "game/all_orders.h"
#include <cstdio>

namespace game {

	Order *Order::construct(Stream &sr) {
		OrderTypeId::Type order_id;
		sr >> order_id;
		if(order_id == OrderTypeId::invalid)
			return nullptr;

		switch(order_id) {
		case OrderTypeId::idle:				return new IdleOrder(sr);
		case OrderTypeId::look_at:			return new LookAtOrder(sr);
		case OrderTypeId::move:				return new MoveOrder(sr);
		case OrderTypeId::track:			return new TrackOrder(sr);
		case OrderTypeId::attack:			return new AttackOrder(sr);
		case OrderTypeId::change_stance:	return new ChangeStanceOrder(sr);
		case OrderTypeId::interact:			return new InteractOrder(sr);
		case OrderTypeId::drop_item:		return new DropItemOrder(sr);
		case OrderTypeId::equip_item:		return new EquipItemOrder(sr);
		case OrderTypeId::unequip_item:		return new UnequipItemOrder(sr);
		case OrderTypeId::transfer_item:	return new TransferItemOrder(sr);
		case OrderTypeId::get_hit:			return new GetHitOrder(sr);
		case OrderTypeId::die:				return new DieOrder(sr);
		default: ASSERT(0);
		}

		return nullptr;
	}


	Order::Order() :m_is_finished(false), m_please_cancel(false) { }
	
	Order::Order(Stream &sr) {
		u8 flags;
		sr >> flags;

		if(flags & 1)
			m_followup.reset(Order::construct(sr));
		m_is_finished = flags & 2;
		m_please_cancel = flags & 4;
	}

	void Order::save(Stream &sr) const {
		u8 flags =	(m_followup? 1 : 0) |
					(m_is_finished? 2 : 0) |
					(m_please_cancel? 4 : 0);
		sr << flags;
		if(m_followup)
			sr << m_followup->typeId() << *m_followup;
	}

	void ThinkingEntity::handleOrder(EntityEvent::Type event, const EntityEventParams &params) {
		if(!m_order)
			return;

		static ThinkingEntity::HandleFunc handlers[] = {
			&ThinkingEntity::handleOrderWrapper<IdleOrder>,
			&ThinkingEntity::handleOrderWrapper<LookAtOrder>,
			&ThinkingEntity::handleOrderWrapper<MoveOrder>,
			&ThinkingEntity::handleOrderWrapper<TrackOrder>,
			&ThinkingEntity::handleOrderWrapper<AttackOrder>,
			&ThinkingEntity::handleOrderWrapper<ChangeStanceOrder>,
			&ThinkingEntity::handleOrderWrapper<InteractOrder>,
			&ThinkingEntity::handleOrderWrapper<DropItemOrder>,
			&ThinkingEntity::handleOrderWrapper<EquipItemOrder>,
			&ThinkingEntity::handleOrderWrapper<UnequipItemOrder>,
			&ThinkingEntity::handleOrderWrapper<TransferItemOrder>,
			&ThinkingEntity::handleOrderWrapper<GetHitOrder>,
			&ThinkingEntity::handleOrderWrapper<DieOrder>
		};
		static_assert(COUNTOF(handlers) == OrderTypeId::count, "Not all order classes are handled in ThinkingEntity::handleOrder");

		if(!m_order->isFinished())
			if(!(this->*handlers[m_order->typeId()])(m_order.get(), event, params))
				m_order->finish();
	}

	bool ThinkingEntity::failOrder() const {
		if(m_ai && m_order)
			m_ai->onFailed(m_order->typeId());
		return false;
	}


}
