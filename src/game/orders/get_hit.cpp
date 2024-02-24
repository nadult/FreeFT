// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/orders/get_hit.h"

#include "game/actor.h"
#include <fwk/math/rotation.h>

namespace game {

GetHitOrder::GetHitOrder(bool has_dodged) : fall_time(0.0f), force_angle(0.0f), force(0.0f) {
	mode = has_dodged ? Mode::dodge : Mode::recoil;
}

GetHitOrder::GetHitOrder(const float3 &force_vec, float fall_time) : fall_time(fall_time) {
	mode = Mode::fall;
	force = length(force_vec);
	float3 vec = force_vec / force;
	force_angle = vectorToAngle(normalize(vec.xz()));
}

GetHitOrder::GetHitOrder(MemoryStream &sr) : OrderImpl(sr) {
	sr.unpack(mode, force, force_angle, fall_time);
}

void GetHitOrder::save(MemoryStream &sr) const {
	Order::save(sr);
	sr.pack(mode, force, force_angle, fall_time);
}

bool Actor::handleOrder(GetHitOrder &order, EntityEvent event, const EntityEventParams &params) {
	typedef GetHitOrder::Mode Mode;

	if(event == EntityEvent::init_order) {
		if(order.mode == Mode::fall) {
			float angle_diff = angleDistance(dirAngle(), order.force_angle);
			animate(angle_diff < pi * 0.5 ? Action::fall_forward : Action::fall_back);
			order.mode = Mode::fall;
			if(order.force >= 10.0f)
				m_target_angle = vectorToAngle(-angleToVector(order.force_angle));
		} else {
			if(order.mode == Mode::dodge) {
				Action::Type action = rand() % 2 == 0 ? Action::dodge1 : Action::dodge2;
				if(!animate(action))
					animate(Action::dodge1);
			} else
				animate(Action::recoil);
		}
	} else if(event == EntityEvent::anim_finished) {
		if(order.mode == Mode::fall) {
			fixPosition();
			animate(m_action == Action::fall_back ? Action::fallen_back : Action::fallen_forward);
			order.mode = Mode::fallen;
		} else if(order.mode != Mode::fallen)
			return false;
	} else if(event == EntityEvent::think && order.mode == Mode::fall) {
		float3 vec = asXZY(angleToVector(order.force_angle), 0.0f);
		float len = timeDelta() * order.force;
		order.force -= timeDelta() * order.force * 4.0f;
		if(order.force < 0.0f)
			order.force = 0.0f;

		if(len > 0.1f) {
			len = min(len, (float)(30.0f * timeDelta()));

			while(len > 0.0f) {
				float tlen = min(len, 1.0f);
				len -= tlen;
				float3 move = vec * tlen;

				FBox new_bbox = boundingBox() + move;
				if(!findAny(new_bbox, {Flags::all | Flags::colliding, ref()}))
					setPos(pos() + move);
				else {
					order.force = 0.0f;
					break;
				}
			}
		}
	} else if(event == EntityEvent::think && order.mode == Mode::fallen) {

		order.fall_time -= timeDelta();
		if(order.fall_time < 0.0f) {
			animate(m_action == Action::fallen_back ? Action::getup_back : Action::getup_forward);
			order.mode = Mode::getup;
		}
	} else if(event == EntityEvent::sound) {
		if(order.mode == Mode::recoil)
			playSound(m_actor.sounds[m_sound_variation].hit, pos());
		else if(order.mode == Mode::fall)
			playSound(m_proto.fall_sound, pos());
		else if(order.mode == Mode::getup)
			playSound(m_actor.sounds[m_sound_variation].get_up, pos());
	}

	return true;
}

}
