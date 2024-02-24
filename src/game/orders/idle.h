// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"

namespace game {

class IdleOrder : public OrderImpl<IdleOrder, OrderTypeId::idle> {
  public:
	IdleOrder();
	IdleOrder(MemoryStream &);

	void save(MemoryStream &) const override;
	void cancel() override;

	float m_fancy_anim_time;
};

}
