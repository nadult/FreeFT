/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/orders/get_hit.h"
#include "game/actor.h"
#include "game/world.h"

namespace game {

	GetHitOrder::GetHitOrder(bool has_dodged) :fall_time(0.0f), force_angle(0.0f) {
		mode = has_dodged? Mode::dodge : Mode::recoil;
	}

	GetHitOrder::GetHitOrder(const float3 &force, float fall_time) :fall_time(fall_time) {
		mode = Mode::fall;
		force_angle	= vectorToAngle((force / length(force)).xz());
	}

	GetHitOrder::GetHitOrder(Stream &sr) :OrderImpl(sr) {
		sr.unpack(mode, force_angle, fall_time);
	}

	void GetHitOrder::save(Stream &sr) const {
		Order::save(sr);
		sr.pack(mode, force_angle, fall_time);
	}

	bool Actor::handleOrder(GetHitOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		typedef GetHitOrder::Mode Mode;

		if(event == ActorEvent::init_order) {
			if(order.mode == Mode::fall) {
				float angle_diff = angleDistance(dirAngle(), order.force_angle);	
				animate(angle_diff < constant::pi * 0.5? Action::fall_forward : Action::fall_back);
				order.mode = Mode::fall;
			}
			else {
				if(order.mode == Mode::dodge) {
					Action::Type action = rand() % 2 == 0? Action::dodge1 : Action::dodge2;
					if(!animate(action))
						animate(Action::dodge1);
				}
				else
					animate(Action::recoil);
			}
		}
		else if(event == ActorEvent::anim_finished) {
			if(order.mode == Mode::fall) {
				animate(m_action == Action::fall_back? Action::fallen_back : Action::fallen_forward);
				order.mode = Mode::fallen;
			}
			else if(order.mode != Mode::fallen)
				return false;
		}
		else if(event == ActorEvent::think && order.mode == Mode::fallen) {
			order.fall_time -= timeDelta();
			if(order.fall_time < 0.0f) {
				animate(m_action == Action::fallen_back? Action::getup_back : Action::getup_forward);
				order.mode = Mode::getup;
			}
		}
		else if(event == ActorEvent::sound) {
			if(order.mode == Mode::recoil)
				world()->playSound(m_actor.sounds[m_sound_variation].hit, pos());
			else if(order.mode == Mode::fall)
				world()->playSound(m_proto.fall_sound, pos());
			else if(order.mode == Mode::getup)
				world()->playSound(m_actor.sounds[m_sound_variation].get_up, pos());
		}

		return true;
	}

}
