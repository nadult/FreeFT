/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_ATTACK_H
#define GAME_ORDERS_ATTACK_H

#include "game/orders.h"

namespace game {


	class AttackOrder: public OrderImpl<AttackOrder, OrderTypeId::attack,
			ActorEvent::init_order | ActorEvent::anim_finished | ActorEvent::fire | ActorEvent::hit | ActorEvent::sound | ActorEvent::next_frame> {
	public:
		AttackOrder(AttackMode::Type mode, const int3 &target_pos);
		AttackOrder(Stream&);

		void save(Stream&) const;
				
		AttackMode::Type m_mode;
		int3 m_target_pos;
		//TODO: target_entity
		
		int m_burst_mode;
		int3 m_burst_off;
	};


}

#endif
