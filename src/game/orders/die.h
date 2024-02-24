// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"

namespace game {

class DieOrder : public OrderImpl<DieOrder, OrderTypeId::die> {
  public:
	DieOrder(DeathId death_id);
	DieOrder(MemoryStream &);

	void save(MemoryStream &) const;

	DeathId m_death_id;
	bool m_is_dead;
};

}
