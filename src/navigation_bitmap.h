#ifndef NAVIGATION_BITMAP_H
#define NAVIGATION_BITMAP_H

#include "tile_map.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"

//TODO: support for multiple levels, stairs: it can be implemented by additional heightmaps
// and transitions between them
//
//TODO: sometimes we can only crouch or be in prone position, because ceiling is low. In nodes
// with varying ceiling height we can provide ceiling heightmaps
class NavigationBitmap {
public:
	NavigationBitmap(const TileMap&);

	int2 size() const { return m_size; }

	gfx::PTexture getTexture() const;
	void printInfo() const;

	inline bool operator()(int x, int y) const
		{ return m_bitmap[(x >> 3) + y * m_line_size] & (1 << (x & 7)); }

	inline bool operator()(const int2 &pos) const
	{ return pos.x < 0 || pos.y < 0 || pos.x >= m_size.x || pos.y >= m_size.y? false : operator()(pos.x, pos.y); }

protected:
	vector<u8> m_bitmap;
	int m_line_size;
	int2 m_size;
};




#endif
