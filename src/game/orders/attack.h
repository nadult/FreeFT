// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"
#include "game/item.h"

namespace game {

	class AttackOrder: public OrderImpl<AttackOrder, OrderTypeId::attack> {
	public:
		//TODO: better attack targets (more user friendly, it's hard to click on moving characters)
		AttackOrder(Maybe<AttackMode>, EntityRef target);
		AttackOrder(Maybe<AttackMode>, const float3 &target_pos);
		AttackOrder(MemoryStream&);

		void save(MemoryStream&) const;

		EntityRef m_target;
		Maybe<AttackMode> m_mode;
		float3 m_target_pos;
		bool m_is_kick_weapon;
		bool m_is_followup;
		
		int m_burst_mode;
	};

}
