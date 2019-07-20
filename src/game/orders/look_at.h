// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"

namespace game {

	class LookAtOrder: public OrderImpl<LookAtOrder, OrderTypeId::look_at> {
	public:
		LookAtOrder(float3);
		LookAtOrder(Stream&);
		void save(Stream&) const;

		float3 m_target;
	};

}
