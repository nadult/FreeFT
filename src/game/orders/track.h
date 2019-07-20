// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/orders.h"
#include "game/path.h"

namespace game {

	class TrackOrder: public OrderImpl<TrackOrder, OrderTypeId::track> {
	public:
		TrackOrder(EntityRef target, float min_distance, bool run);
		TrackOrder(Stream&);

		void save(Stream&) const;

		int m_next_update;
		EntityRef m_target;
		PathPos m_path_pos;
		Path m_path;

		float m_time_for_update;
		float m_min_distance;
		bool m_please_run;
	};

}
