// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"
#include "game/path.h"

namespace game {

class MoveOrder : public OrderImpl<MoveOrder, OrderTypeId::move> {
  public:
	MoveOrder(const int3 &target_pos, bool run);
	MoveOrder(MemoryStream &);

	void save(MemoryStream &) const;

	int3 m_target_pos;
	Path m_path;
	PathPos m_path_pos;
	bool m_please_run;
};

}
