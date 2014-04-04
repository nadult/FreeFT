/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/move.h"
#include "game/actor.h"
#include "game/world.h"
#include "net/socket.h"

namespace game {

	MoveOrder::MoveOrder(const int3 &target_pos, bool run)
		:m_target_pos(target_pos), m_please_run(run) { }

	MoveOrder::MoveOrder(Stream &sr) :OrderImpl(sr) {
		m_target_pos = net::decodeInt3(sr);
		sr >> m_please_run >> m_path >> m_path_pos;
	}

	void MoveOrder::save(Stream &sr) const {
		OrderImpl::save(sr);
		net::encodeInt3(sr, m_target_pos);
		sr << m_please_run << m_path << m_path_pos;
	}	

	bool Actor::handleOrder(MoveOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			int3 cur_pos = (int3)pos();
			if(cur_pos == order.m_target_pos)
				return false;

			if(!world()->findPath(order.m_path, cur_pos, order.m_target_pos, ref()))
				return false;
		
			if(order.m_please_run && m_proto.simpleAnimId(Action::run, m_stance) == -1)
				order.m_please_run = 0;

			if(!animate(order.m_please_run? Action::run : Action::walk))
				return false;
		}
		if(event == ActorEvent::think) {
			if(order.needCancel()) {
				fixPosition();
				return false;
			}
			if(followPath(order.m_path, order.m_path_pos, order.m_please_run) != FollowPathResult::moved)
				return false;
		}
		if(event == ActorEvent::anim_finished && m_stance == Stance::crouch) {
			animate(m_action);
		}
		if(event == ActorEvent::step) {
			SurfaceId::Type standing_surface = surfaceUnder();
			world()->playSound(m_proto.step_sounds[m_stance][standing_surface], pos());
		}

		return true;
	}

}
