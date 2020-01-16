// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

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
		InteractOrder(MemoryStream&);

		void save(MemoryStream&) const;
		
		EntityRef m_target;
		Maybe<InteractionMode> m_mode;
		bool m_is_followup;
	};

}
