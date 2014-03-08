/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_MOVE_H
#define GAME_ORDERS_MOVE_H

#include "game/orders.h"

namespace game {

	class MoveOrder: public OrderImpl<MoveOrder, OrderTypeId::move,
			ActorEvent::init_order | ActorEvent::think | ActorEvent::step | ActorEvent::anim_finished> {
	public:
		MoveOrder(const int3 &target_pos, bool run);
		MoveOrder(Stream&);

		void save(Stream&) const;

		int3 m_target_pos;
		bool m_please_run;

		int3 m_last_pos;
		float m_path_t;
		int m_path_pos;
		vector<int3> m_path;
	};

}

#endif
