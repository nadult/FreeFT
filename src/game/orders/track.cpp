// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/track.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	TrackOrder::TrackOrder(EntityRef target, float min_distance, bool run)
	  :m_target(target), m_min_distance(min_distance), m_please_run(run), m_time_for_update(0.0f) {
	}

	TrackOrder::TrackOrder(MemoryStream &sr) :OrderImpl(sr) {
		m_target.load(sr);
		sr >> m_min_distance >> m_please_run;
		m_path.load(sr);
		m_path_pos.load(sr);
		sr >> m_time_for_update;
	}

	void TrackOrder::save(MemoryStream &sr) const {
		Order::save(sr);
		m_target.save(sr);
		sr << m_min_distance << m_please_run;
		m_path.save(sr);
		m_path_pos.save(sr);
		sr << m_time_for_update;
	}

	bool Actor::handleOrder(TrackOrder &order, EntityEvent event, const EntityEventParams &params) {
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

				if(!world()->findClosestPos(target_pos, cur_pos, encloseIntegral(target_box), ref()))
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
				SurfaceId standing_surface = surfaceUnder();
				playSound(m_proto.step_sounds[m_stance][standing_surface], pos());
			}
		}

		return true;
	}


}
