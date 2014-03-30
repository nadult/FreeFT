/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NAVI_MAP_H
#define NAVI_MAP_H

#include "base.h"

class NaviHeightmap;

//TODO: sometimes we can only crouch or be in prone position, because ceiling is low. In nodes
// with varying ceiling height we can provide ceiling heightmaps
class NaviMap {
public:
	enum {
		sector_size = 32,
	};

	NaviMap(int agent_size);

	void update(const NaviHeightmap&);

	int2 dimensions() const { return m_size; }
	int agentSize() const { return m_agent_size; }

	void visualize(gfx::SceneRenderer&, bool borders) const;
	void visualizePath(const vector<int3>&, int agent_size, gfx::SceneRenderer&) const;
	void printInfo() const;

	struct Quad {
		Quad(const IRect &rect, int min_height, int max_height)
			:rect(rect), is_disabled(0), static_ncount(0), min_height(min_height), max_height(max_height) { }
		Quad() { }

		IRect rect;
		vector<int> neighbours;
		int static_ncount: 31;
		int is_disabled : 1;

		int min_height, max_height;
	} __attribute__((aligned(64)));

	struct PathNode {
		int2 point;
		int quad_id;
	};

	int3 findClosestCorrectPos(const int3 &source, const IBox &target) const;
	int findQuad(const int3 &pos, bool find_disabled = false) const;

	void addCollider(const IRect &rect);
	void removeColliders();

	vector<int3> findPath(const int3 &start, const int3 &end) const;

	int quadCount() const { return (int)m_quads.size(); }
	const Quad &operator[](int idx) const { return m_quads[idx]; }

protected:
	vector<PathNode> findPath(const int2 &start, const int2 &end, int start_id, int end_id, bool do_refining) const;

	void extractQuads(const PodArray<u8>&, int sx, int sy);
	void addAdjacencyInfo(int target_id, int src_id);
	void addCollider(int quad_id, const IRect &rect);

	int m_agent_size;
	int m_static_count;
	vector<Quad> m_quads;
	int2 m_size;
};


#endif
