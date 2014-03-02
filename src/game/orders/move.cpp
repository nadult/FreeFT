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
		sr >> m_target_pos >> m_please_run;

		sr >> m_path_t;
		m_path_pos = sr.decodeInt();
		m_last_pos = net::decodeInt3(sr);
		int3 prev = m_last_pos;
		int path_size = sr.decodeInt();
		m_path.resize(path_size);
		for(int i = 0; i < path_size; i++) {
			m_path[i] = net::decodeInt3(sr) + prev;
			prev = m_path[i];
		}
	}

	void MoveOrder::save(Stream &sr) const {
		OrderImpl::save(sr);

		sr << m_target_pos << m_please_run;
		sr << m_path_t;
		sr.encodeInt(m_path_pos);
		net::encodeInt3(sr, m_last_pos);
		sr.encodeInt(m_path.size());
		int3 prev = m_last_pos;
		for(int i = 0; i < (int)m_path.size(); i++) {
			net::encodeInt3(sr, m_path[i] - prev);
			prev = m_path[i];
		}
	}	

	void Actor::handleOrder(MoveOrder &order, ActorEvent::Type event, const ActorEventParams &params) {
		if(event == ActorEvent::init_order) {
			int3 new_pos = order.m_target_pos;

			new_pos = max(new_pos, int3(0, 0, 0)); //TODO: clamp to map extents

			int3 cur_pos = (int3)pos();
			vector<int3> tmp_path = world()->findPath(cur_pos, new_pos);

			if(cur_pos == new_pos || tmp_path.empty()) {
				order.finish();
				return;
			}

			order.m_last_pos = cur_pos;
			order.m_path.clear();
			order.m_path_t = 0;
			order.m_path_pos = 0;
			roundPos();

			for(int n = 1; n < (int)tmp_path.size(); n++) {
				cur_pos = tmp_path[n - 1];
				new_pos = tmp_path[n];
				if(new_pos == cur_pos)
					continue;

				MoveVector mvec(tmp_path[n - 1].xz(), tmp_path[n].xz());

				while(mvec.dx) {
					int step = min(mvec.dx, 3);
					order.m_path.push_back(cur_pos += int3(mvec.vec.x * step, 0, 0));
					mvec.dx -= step;
				}
				while(mvec.dy) {
					int step = min(mvec.dy, 3);
					order.m_path.push_back(cur_pos += int3(0, 0, mvec.vec.y * step));
					mvec.dy -= step;
				}
				while(mvec.ddiag) {
					int dstep = min(mvec.ddiag, 3);
					order.m_path.push_back(cur_pos += asXZ(mvec.vec) * dstep);
					mvec.ddiag -= dstep;
				}
			}
			
			if(order.m_path.size() <= 1 || m_stance != Stance::standing)
				order.m_please_run = 0;
			animate(order.m_please_run? ActionId::running : ActionId::walking);
			DASSERT(!order.m_path.empty());
		}
		if(event == ActorEvent::think) {
			DASSERT(!order.m_path.empty() && order.m_path_pos >= 0 && order.m_path_pos < (int)order.m_path.size());
			
			float speed = m_actor.speeds[order.m_please_run? 3 : m_stance];
			float dist = speed * timeDelta();
			float3 new_pos;

			while(dist > 0.0001f) {
				int3 target = order.m_path[order.m_path_pos];
				if(findAny(boundingBox() - pos() + float3(target), this, collider_tiles))
					target.y += 1;

				int3 diff = target - order.m_last_pos;
				float3 diff_vec(diff); diff_vec = diff_vec / length(diff_vec);
				float3 cur_pos = float3(order.m_last_pos) + float3(diff) * order.m_path_t;
				float tdist = distance(float3(target), cur_pos);

				if(tdist < dist) {
					dist -= tdist;
					order.m_last_pos = target;
					order.m_path_t = 0.0f;

					if(++order.m_path_pos == (int)order.m_path.size() || order.needCancel()) {
						lookAt(target);
						new_pos = target;
						order.finish();
						break;
					}
				}
				else {
					float new_x = cur_pos.x + diff_vec.x * dist;
					float new_z = cur_pos.z + diff_vec.z * dist;
					order.m_path_t = diff.x?	(new_x - order.m_last_pos.x) / float(diff.x) :
												(new_z - order.m_last_pos.z) / float(diff.z);
					new_pos = (float3)order.m_last_pos + float3(diff) * order.m_path_t;
					lookAt(target);
					break;
				}
			}

			if(findAny(boundingBox() + new_pos - pos(), this, collider_dynamic | collider_dynamic_nv)) {
				//TODO: response to collision
				order.finish();
			}
			else
				setPos(new_pos);
		}
		if(event == ActorEvent::anim_finished && m_stance == Stance::crouching) {
			animate(m_action_id);
		}
		if(event == ActorEvent::step) {
			SurfaceId::Type standing_surface = surfaceUnder();
			world()->playSound(m_proto.step_sounds[m_stance][standing_surface], pos());
		}
	}

}
