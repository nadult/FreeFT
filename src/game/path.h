// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/base.h"

namespace game {

	struct PathPos {
		PathPos() :node_id(0), delta(0.0f) { }
		void save(MemoryStream&) const;
		void load(MemoryStream&);

		int node_id;
		float delta;
	};

	class Path {
	public:
		// Each node can differ from previous one only 
		// by a single component (x, y, or diagonal)
		// At least two points are needed for a valid path
		Path(const vector<int3> &nodes);
		Path() = default;

		void save(MemoryStream&) const;
		void load(MemoryStream&);

		// Returns true if finished
		// TODO: take current position (actual position, not computed from PathPos) into consideration
		bool follow(PathPos &pos, float step) const;
		float3 pos(const PathPos &pos) const;

		bool isValid(const PathPos&) const;
		bool empty() const { return m_nodes.size() <= 1; }
		void visualize(int agent_size, SceneRenderer&) const;

		float length() const;
		float length(const PathPos&) const;

	private:
		vector<int3> m_nodes;
	};

}
