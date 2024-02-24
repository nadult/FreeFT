// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders.h"
#include "game/actor.h"
#include "game/all_orders.h"
#include "game/brain.h"
#include "game/container.h"
#include "game/world.h"
#include <cstdio>

namespace game {

Order *Order::construct(MemoryStream &sr) {
	OrderTypeId order_id;
	sr >> order_id;

	switch(order_id) {
	case OrderTypeId::idle:
		return new IdleOrder(sr);
	case OrderTypeId::look_at:
		return new LookAtOrder(sr);
	case OrderTypeId::move:
		return new MoveOrder(sr);
	case OrderTypeId::track:
		return new TrackOrder(sr);
	case OrderTypeId::attack:
		return new AttackOrder(sr);
	case OrderTypeId::change_stance:
		return new ChangeStanceOrder(sr);
	case OrderTypeId::interact:
		return new InteractOrder(sr);
	case OrderTypeId::drop_item:
		return new DropItemOrder(sr);
	case OrderTypeId::equip_item:
		return new EquipItemOrder(sr);
	case OrderTypeId::unequip_item:
		return new UnequipItemOrder(sr);
	case OrderTypeId::transfer_item:
		return new TransferItemOrder(sr);
	case OrderTypeId::get_hit:
		return new GetHitOrder(sr);
	case OrderTypeId::die:
		return new DieOrder(sr);
	default:
		ASSERT(0);
	}

	return nullptr;
}

Order::Order() : m_is_finished(false), m_please_cancel(false) {}

Order::Order(MemoryStream &sr) {
	u8 flags;
	sr >> flags;

	if(flags & 1)
		m_followup.reset(Order::construct(sr));
	m_is_finished = flags & 2;
	m_please_cancel = flags & 4;
}

void Order::save(MemoryStream &sr) const {
	u8 flags = (m_followup ? 1 : 0) | (m_is_finished ? 2 : 0) | (m_please_cancel ? 4 : 0);
	sr << flags;
	if(m_followup) {
		sr << m_followup->typeId();
		m_followup->save(sr);
	}
}

void ThinkingEntity::handleOrder(EntityEvent event, const EntityEventParams &params) {
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
		&ThinkingEntity::handleOrderWrapper<DieOrder>};
	static_assert(arraySize(handlers) == count<OrderTypeId>,
				  "Not all order classes are handled in ThinkingEntity::handleOrder");

	if(!m_order->isFinished())
		if(!(this->*handlers[(int)m_order->typeId()])(m_order.get(), event, params))
			m_order->finish();
}

bool ThinkingEntity::failOrder() const {
	if(m_ai && m_order)
		m_ai->onFailed(m_order->typeId());
	return false;
}

}
