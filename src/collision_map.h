#ifndef COLLISION_MAP_H
#define COLLISION_MAP_H

#include "tile_map.h"
#include "gfx/device.h"

//TODO: support for multiple levels, stairs: it can be implemented by additional heightmaps
// and transitions between them
//
//TODO: sometimes we can only crouch or be in prone position, because ceiling is low. In nodes
// with varying ceiling height we can provide ceiling heightmaps
class CollisionMap {
public:
	CollisionMap(int2 size);

	void update(const TileMap&);

	void resize(int2 size);
	int2 size() const { return m_size; }

	gfx::PTexture getTexture() const;
	void printInfo() const;

	vector<IRect> extractQuads();

	inline bool operator()(int x, int y) const
		{ return m_bitmap[(x >> 3) + y * m_line_size] & (1 << (x & 7)); }

protected:
	vector<u8> m_bitmap;
	int2 m_size;
	int m_line_size;
};


#endif
