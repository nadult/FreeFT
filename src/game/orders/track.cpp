/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/track.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	TrackOrder::TrackOrder(EntityRef target, float min_distance, bool run)
	  :m_target(target), m_min_distance(min_distance), m_please_run(run), m_time_for_update(0.0f) {
	}

	TrackOrder::TrackOrder(Stream &sr) :OrderImpl(sr) {
		sr >> m_target >> m_min_distance >> m_please_run;
		sr >> m_path >> m_path_pos >> m_time_for_update;
	}

	void TrackOrder::save(Stream &sr) const {
		Order::save(sr);
		sr << m_target << m_min_distance << m_please_run;
		sr << m_path << m_path_pos << m_time_for_update;
	}

	bool Actor::handleOrder(TrackOrder &order, EntityEvent::Type event, const EntityEventParams &params) {
		if(event == EntityEvent::init_order || event == EntityEvent::think) {
			order.m_time_for_update -= timeDelta();
			const Entity *target = refEntity(order.m_target);
			if(!target)
				return false;

			if(order.m_time_for_update < 0.0f) {
				fixPosition();

				int3 cur_pos = (int3)pos();

				int3 target_pos;
				FBox target_box = target->boundingBox();

				if(!world()->findClosestPos(target_pos, cur_pos, enclosingIBox(target_box), ref()))
					return failOrder();

				order.m_path_pos = PathPos();
				if(!world()->findPath(order.m_path, cur_pos, target_pos, ref()))
					return failOrder();
				order.m_time_for_update = 0.25f;
			}
			
			if(distance(boundingBox(), target->boundingBox()) <= order.m_min_distance)
				return false;

			if(event == EntityEvent::init_order) {
				if(order.m_please_run && m_proto.simpleAnimId(Action::run, m_stance) == -1)
					order.m_please_run = 0;

				if(!animate(order.m_please_run? Action::run : Action::walk))
					return failOrder();
			}

			FollowPathResult result = order.needCancel()? FollowPathResult::finished :
				followPath(order.m_path, order.m_path_pos, order.m_please_run);	

			if(result != FollowPathResult::moved)
				return false;
		}
		
		if(order.m_path.length(order.m_path_pos) > 1.0f) {
			if(event == EntityEvent::anim_finished && m_stance == Stance::crouch) {
				animate(m_action);
			}
			if(event == EntityEvent::step) {
				SurfaceId::Type standing_surface = surfaceUnder();
				playSound(m_proto.step_sounds[m_stance][standing_surface], pos());
			}
		}

		return true;
	}


}
