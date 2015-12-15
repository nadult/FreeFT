/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/base.h"
#include "gfx/scene_renderer.h"
#include "game/path.h"
#include "net/socket.h"

namespace game {

	void PathPos::save(Stream &sr) const {
		encodeInt(sr, node_id);
		sr << delta;
	}

	void PathPos::load(Stream &sr) {
		node_id = decodeInt(sr);
		sr >> delta;
	}
		
	bool Path::isValid(const PathPos &pos) const {
		return	pos.node_id >= 0 && pos.node_id < (int)m_nodes.size() &&
				pos.delta >= 0.0f && pos.delta <= 1.0f;
	}

	Path::Path(const vector<int3> &path) :m_nodes(path) {
#ifndef NDEBUG
		for(int n = 1; n < (int)path.size(); n++) {
			MoveVector move(path[n].xz(), path[n - 1].xz());
			int components = (move.dx? 1 : 0) + (move.dy? 1 : 0) + (move.ddiag? 1 : 0);
			DASSERT(components == 1);
		}
#endif
	}
		
	bool Path::follow(PathPos &path_pos, float step) const {
		DASSERT(isValid(path_pos));

		if(path_pos.node_id == (int)m_nodes.size() - 1) {
			path_pos.delta = 0.0f;
			return true;
		}

		while(step > 0.0f) {
			int3 prev_node = m_nodes[path_pos.node_id], next_node = m_nodes[path_pos.node_id + 1];

			float3 diff = next_node - prev_node;
			float diff_len = ::length(diff);
			float3 diff_vec = diff / diff_len;

			float3 cur_pos = float3(prev_node) + float3(diff) * path_pos.delta;
			float next_dist = distance(float3(next_node), cur_pos);

			path_pos.delta += step / diff_len;

			if(path_pos.delta >= 1.0f) {
				step = (path_pos.delta - 1.0f) * diff_len;
				path_pos.delta = 0.0f;
				path_pos.node_id++;
				if(path_pos.node_id == (int)m_nodes.size() - 1)
					return true;
			}
			else
				break;
		}

		return false;
	}

	float3 Path::pos(const PathPos &path_pos) const {
		DASSERT(isValid(path_pos));
		int last_node_id = (int)m_nodes.size() - 1;

		float3 p1(m_nodes[path_pos.node_id]);
		float3 p2(m_nodes[path_pos.node_id == last_node_id? last_node_id : path_pos.node_id + 1]);

		return p1 + (p2 - p1) * path_pos.delta;
	}

	void Path::save(Stream &sr) const {
		encodeInt(sr, (int)m_nodes.size());
		if(m_nodes.empty())
			return;

		net::encodeInt3(sr, m_nodes.front());
		for(int n = 1; n < (int)m_nodes.size(); n++)
			net::encodeInt3(sr, m_nodes[n] - m_nodes[n - 1]);
	}

	void Path::load(Stream &sr) {
		int count = decodeInt(sr);
		ASSERT(count >= 0);
		m_nodes.resize(count);

		if(m_nodes.empty())
			return;

		m_nodes.front() = net::decodeInt3(sr);
		for(int n = 1; n < (int)m_nodes.size(); n++) {
			int3 diff = net::decodeInt3(sr);
			m_nodes[n] = m_nodes[n - 1] + diff;
		}
	}
	
	void Path::visualize(int agent_size, SceneRenderer &renderer) const {
		if(m_nodes.empty())
			return;

		IBox box(0, 0, 0, agent_size, 0, agent_size);
		renderer.addBox(box + m_nodes.front(), Color::red);

		for(int n = 1; n < (int)m_nodes.size(); n++) {
			int3 begin = m_nodes[n - 1], end = m_nodes[n];
			bool first = true;

			IBox start_box = box + begin, end_box = box + end;
			renderer.addBox(end_box, Color::red);
			MoveVector vec(begin.xz(), end.xz());

			if(vec.vec == int2(1, 1) || vec.vec == int2(-1, -1)) {
				swap(start_box.min.x, start_box.max.x);
				swap(  end_box.min.x,   end_box.max.x);
			}

			renderer.addLine(start_box.min, end_box.min);
			renderer.addLine(start_box.max, end_box.max);
		}
	}

	float Path::length() const {
		float out = 0.0f;
		for(int n = 1; n < (int)m_nodes.size(); n++)
			out += distance((float3)m_nodes[n - 1], (float3)m_nodes[n]);
		return out;
	}

	float Path::length(const PathPos &pos) const {
		if(!isValid(pos))
			return 0.0f;

		float out = 0.0f;
		for(int n = 1; n < (int)pos.node_id; n++)
			out += distance((float3)m_nodes[n - 1], (float3)m_nodes[n]);

		if(pos.node_id < (int)m_nodes.size() - 1)
			out += distance(m_nodes[pos.node_id], m_nodes[pos.node_id + 1]) * pos.delta;
		return out;
	}

}

