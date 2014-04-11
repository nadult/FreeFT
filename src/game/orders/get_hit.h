/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_GET_HIT_H
#define GAME_ORDERS_GET_HIT_H

#include "game/orders.h"

namespace game {

	//TODO: if get hit order was issued while performing some action involving
	// crouching animation (for example: pick up) we might change stance to crouch
	// when falling
	class GetHitOrder: public OrderImpl<GetHitOrder, OrderTypeId::get_hit> {
	public:
		GetHitOrder(bool has_dodged);
		GetHitOrder(const float3 &force, float fall_time);
		GetHitOrder(Stream&);

		void save(Stream&) const;

			
		enum class Mode: char {
			recoil,
			dodge,
			fall,
			fallen,
			getup,
		};

		Mode mode;
		float fall_time;
		float force_angle;
	};

}

#endif
