#ifndef NAVIGATION_MAP_H
#define NAVIGATION_MAP_H

#include "tile_map.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"

class NavigationBitmap;

//TODO: support for multiple levels, stairs: it can be implemented by additional heightmaps
// and transitions between them
//
//TODO: sometimes we can only crouch or be in prone position, because ceiling is low. In nodes
// with varying ceiling height we can provide ceiling heightmaps
class NavigationMap {
public:
	NavigationMap();

	void update(const NavigationBitmap&);
	int2 size() const { return m_size; }

	void visualize(gfx::SceneRenderer&, bool borders) const;
	void visualizePath(const vector<int2>&, int elem_size, gfx::SceneRenderer&) const;
	void printInfo() const;

	struct Quad {
		IRect rect;
		vector<int> neighbours;
		vector<IRect> edges;

		mutable int src_quad;
		mutable float dist, edist;
		mutable int2 entry_pos;
		mutable bool is_finished;
	};

	struct PathNode {
		int2 point;
		int quad_id;
	};

	int2 findClosestCorrectPos(const int2 &pos, const IRect &dist_to) const;
	int findQuad(int2 pos) const;

	vector<PathNode> findPath(int2 start, int2 end, bool do_refining) const;
	vector<int2> findPath(int2 start, int2 end) const;

	int quadCount() const { return (int)m_quads.size(); }
	const Quad &operator[](int idx) const { return m_quads[idx]; }

protected:
	void extractQuads(const NavigationBitmap&, int sx, int sy);

	vector<Quad> m_quads;
	int2 m_size;
};


#endif
