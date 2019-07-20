// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_ORDERS_INTERACT_H
#define GAME_ORDERS_INTERACT_H

#include "game/orders.h"
#include "game/entity.h"

namespace game {

	DEFINE_ENUM(InteractionMode,
		normal,
		pickup,
		use_item
	);

	class InteractOrder: public OrderImpl<InteractOrder, OrderTypeId::interact> {
	public:
		InteractOrder(EntityRef target, Maybe<InteractionMode> mode = none);
		InteractOrder(Stream&);

		void save(Stream&) const;
		
		EntityRef m_target;
		Maybe<InteractionMode> m_mode;
		bool m_is_followup;
	};

}

#endif
