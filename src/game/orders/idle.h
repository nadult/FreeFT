// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_ORDERS_IDLE_H
#define GAME_ORDERS_IDLE_H

#include "game/orders.h"

namespace game {

	class IdleOrder: public OrderImpl<IdleOrder, OrderTypeId::idle> {
	public:
		IdleOrder();
		IdleOrder(Stream&);

		void save(Stream&) const override;
		void cancel() override;

		float m_fancy_anim_time;
	};

}

#endif
