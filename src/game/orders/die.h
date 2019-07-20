// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_ORDERS_DIE_H
#define GAME_ORDERS_DIE_H

#include "game/orders.h"

namespace game {

	class DieOrder: public OrderImpl<DieOrder, OrderTypeId::die> {
	public:
		DieOrder(DeathId death_id);
		DieOrder(Stream&);

		void save(Stream&) const;
		
		DeathId m_death_id;
		bool m_is_dead;
	};

}

#endif
