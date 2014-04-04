/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_TRACK_H
#define GAME_ORDERS_TRACK_H

#include "game/orders.h"

namespace game {

	class TrackOrder: public OrderImpl<TrackOrder, OrderTypeId::track,
			ActorEvent::init_order | ActorEvent::think | ActorEvent::step | ActorEvent::anim_finished> {
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

#endif
