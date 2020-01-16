// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"

namespace game {

	class ChangeStanceOrder: public OrderImpl<ChangeStanceOrder, OrderTypeId::change_stance> {
	public:
		ChangeStanceOrder(Stance target_stance);
		ChangeStanceOrder(MemoryStream&);

		void save(MemoryStream&) const;

		Stance m_target_stance;
		bool m_stance_up;
	};

}
