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

	int2 dimensions() const { return m_size * sector_size; }
	int agentSize() const { return m_agent_size; }

	void visualize(gfx::SceneRenderer&, bool borders) const;
	void visualizePath(const vector<int3>&, int agent_size, gfx::SceneRenderer&) const;
	void printInfo() const;

	struct Quad {
		Quad(const IRect &rect, u8 min_height, u8 max_height)
			:rect(rect), is_disabled(0), static_ncount(0), collider_id(-1), min_height(min_height), max_height(max_height) { }
		Quad() { }

		const IBox box() const { return IBox(asXZY(rect.min, (int)min_height), asXZY(rect.max, (int)max_height)); }

		IRect rect;
		vector<int> neighbours;
		ListNode node;
		int static_ncount: 31;
		int is_disabled : 1;
		int collider_id;
		u8 min_height, max_height;
	} __attribute__((aligned(64)));

	static_assert(sizeof(Quad) == 64, "");

	struct PathNode {
		int2 point;
		int quad_id;
	};

	int3 findClosestCorrectPos(const int3 &source, const IBox &target) const;
	int findQuad(const int3 &pos, int filter_collider = -1) const;
	void findQuads(const IBox &box, vector<int> &out, bool cheap_filter = true) const;

	void addCollider(const IBox &box, int collider_id);
	void removeColliders();

	vector<int3> findPath(const int3 &start, const int3 &end, int filter_collider = -1) const;

	int quadCount() const { return (int)m_quads.size(); }
	const Quad &operator[](int idx) const { return m_quads[idx]; }

protected:
	vector<PathNode> findPath(const int2 &start, const int2 &end, int start_id, int end_id, bool do_refining,
								int filter_collider) const;
	
	const int findSector(const int2 &xz) { return xz.x / sector_size + xz.y / sector_size * m_size.x; }

	void extractQuads(const PodArray<u8>&, const int2 &bsize, int sx, int sy);
	void addAdjacencyInfo(int target_id, int src_id);
	void addCollider(int quad_id, const IRect &rect, int collider_id);

	int m_agent_size;
	int m_static_count;
	vector<Quad> m_quads;
	vector<List> m_sectors;
	int2 m_size; // in sectors
};


#endif
