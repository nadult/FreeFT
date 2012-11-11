#ifndef NAVIGATION_MAP_H
#define NAVIGATION_MAP_H

#include "tile_map.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"

//TODO: support for multiple levels, stairs: it can be implemented by additional heightmaps
// and transitions between them
//
//TODO: sometimes we can only crouch or be in prone position, because ceiling is low. In nodes
// with varying ceiling height we can provide ceiling heightmaps
class NavigationMap {
public:
	NavigationMap(int2 size);

	void update(const TileMap&);

	void resize(int2 size);
	int2 size() const { return m_size; }

	gfx::PTexture getTexture() const;
	void visualize(gfx::SceneRenderer&, bool borders) const;
	void printInfo() const;

	inline bool operator()(int x, int y) const
		{ return m_bitmap[(x >> 3) + y * m_line_size] & (1 << (x & 7)); }

	struct Quad {
		IRect rect;
		vector<int> neighbours;
		mutable int src_quad, dist;
		mutable int2 entry_pos;
		mutable bool is_finished;
	};

	int findQuad(int2 pos) const;
	vector<int2> findPath(int2 start, int2 end) const;

protected:

	void extractQuads();

	vector<Quad> m_quads;
	vector<u8> m_bitmap;
	int2 m_size;
	int m_line_size;
};


#endif
