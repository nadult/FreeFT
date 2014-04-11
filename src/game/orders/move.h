/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_MOVE_H
#define GAME_ORDERS_MOVE_H

#include "game/orders.h"
#include "game/path.h"

namespace game {

	class MoveOrder: public OrderImpl<MoveOrder, OrderTypeId::move> {
	public:
		MoveOrder(const int3 &target_pos, bool run);
		MoveOrder(Stream&);

		void save(Stream&) const;

		int3 m_target_pos;
		Path m_path;
		PathPos m_path_pos;
		bool m_please_run;
	};

}

#endif
