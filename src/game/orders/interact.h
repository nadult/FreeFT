/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_INTERACT_H
#define GAME_ORDERS_INTERACT_H

#include "game/orders.h"
#include "game/entity.h"

namespace game {

	DECLARE_ENUM(InteractionMode,
		undefined = -1,
		normal,
		pickup,
		use_item
	);

	class InteractOrder: public OrderImpl<InteractOrder, OrderTypeId::interact,
			ActorEvent::init_order | ActorEvent::anim_finished | ActorEvent::pickup> {
	public:
		InteractOrder(EntityRef target, InteractionMode::Type mode = InteractionMode::undefined);
		InteractOrder(Stream&);

		void save(Stream&) const;
		
		EntityRef m_target;
		InteractionMode::Type m_mode;
		bool m_is_followup;
	};

}

#endif
