/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor.h"
#include "game/orders.h"
#include "game/container.h"
#include "game/world.h"
#include <cstdio>

namespace game {


	DEFINE_ENUM(OrderTypeId,
		"idle",
		"move",
		"attack",
		"change_stance",
		"interact",
		"drop_item",
		"equip_item",
		"unequip_item",
		"transfer_item",
		"die"
	);

	void POrder::save(Stream &sr) const {
		sr << (isValid()? get()->typeId() : OrderTypeId::invalid);
		if(isValid())
			get()->save(sr);
	}

	void POrder::load(Stream &sr) {
		reset(Order::construct(sr));
	}

	Order *Order::construct(Stream &sr) {
		OrderTypeId::Type order_id;
		sr >> order_id;
		if(order_id == OrderTypeId::invalid)
			return nullptr;

		switch(order_id) {
		case OrderTypeId::idle:				return new IdleOrder(sr);
		case OrderTypeId::look_at:			return new LookAtOrder(sr);
		case OrderTypeId::move:				return new MoveOrder(sr);
		case OrderTypeId::attack:			return new AttackOrder(sr);
		case OrderTypeId::change_stance:	return new ChangeStanceOrder(sr);
		case OrderTypeId::interact:			return new InteractOrder(sr);
		case OrderTypeId::drop_item:		return new DropItemOrder(sr);
		case OrderTypeId::equip_item:		return new EquipItemOrder(sr);
		case OrderTypeId::unequip_item:		return new UnequipItemOrder(sr);
		case OrderTypeId::transfer_item:	return new TransferItemOrder(sr);
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
			sr >> m_followup;
		m_is_finished = flags & 2;
		m_please_cancel = flags & 4;
	}

	void Order::save(Stream &sr) const {
		u8 flags =	(m_followup? 1 : 0) |
					(m_is_finished? 2 : 0) |
					(m_please_cancel? 4 : 0);
		sr << flags;
		if(m_followup)
			sr << m_followup;
	}

	void Actor::updateOrderFunc() {
		static HandleFunc s_handle_funcs[] = {
			&Actor::handleOrder<IdleOrder>,
			&Actor::handleOrder<LookAtOrder>,
			&Actor::handleOrder<MoveOrder>,
			&Actor::handleOrder<AttackOrder>,
			&Actor::handleOrder<ChangeStanceOrder>,
			&Actor::handleOrder<InteractOrder>,
			&Actor::handleOrder<DropItemOrder>,
			&Actor::handleOrder<EquipItemOrder>,
			&Actor::handleOrder<UnequipItemOrder>,
			&Actor::handleOrder<TransferItemOrder>,
			&Actor::handleOrder<DieOrder>
		};
		static_assert(COUNTOF(s_handle_funcs) == OrderTypeId::count, "");

		m_order_func = m_order? s_handle_funcs[m_order->typeId()] : &Actor::emptyHandleFunc;
	}

}
