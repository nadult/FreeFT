/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_CHANGE_STANCE_H
#define GAME_ORDERS_CHANGE_STANCE_H

#include "game/orders.h"

namespace game {

	class ChangeStanceOrder: public OrderImpl<ChangeStanceOrder, OrderTypeId::change_stance> {
	public:
		ChangeStanceOrder(Stance target_stance);
		ChangeStanceOrder(Stream&);

		void save(Stream&) const;

		Stance m_target_stance;
		bool m_stance_up;
	};

}

#endif
