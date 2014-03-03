/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_DIE_H
#define GAME_ORDERS_DIE_H

#include "game/orders.h"

namespace game {

	class DieOrder: public OrderImpl<DieOrder, OrderTypeId::die,
			ActorEvent::init_order | ActorEvent::anim_finished | ActorEvent::sound> {
	public:
		DieOrder(DeathTypeId::Type death_id);
		DieOrder(Stream&);

		void save(Stream&) const;
		
		DeathTypeId::Type m_death_id;
	};

}

#endif
