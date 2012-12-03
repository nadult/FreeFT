#include "navigation_bitmap.h"
#include "gfx/texture.h"
#include <cstring>
#include <cmath>
#include <set>
#include <algorithm>

static void extractheightMap(const TileMap &tile_map, u8 *out, int2 size, int extend) {
	DASSERT(out);
	memset(out, 0, size.x * size.y);
	IBox dbox(int3(0, 0, 0), int3(size.x, 255, size.y));
	
	for(int t = 0; t < tile_map.nodeCount(); t++) {
		const TileMapNode &node = tile_map(t);
		int3 tnode_pos = tile_map.nodePos(t);
		
		for(int i = 0; i < node.instanceCount(); i++) {
			const TileInstance &inst = node(i);
			IBox bbox = inst.boundingBox() + tnode_pos;
			
			if(areOverlapping(dbox, bbox)) {
				bbox.min = max(bbox.min - int3(extend, 0, extend), int3(0, 0, 0));
				bbox.max = min(bbox.max, dbox.max);

				u8 *ptr = out + bbox.min.x + bbox.min.z * size.x;
				for(int z = 0; z < bbox.depth(); z++) {
					for(int x = 0; x < bbox.width(); x++)
						ptr[x] = max(ptr[x], (u8)bbox.max.y);
					ptr += size.x;
				}
			}
		}
	}
}

NavigationBitmap::NavigationBitmap(const TileMap &tile_map) :m_size(0, 0) {
	m_size = tile_map.size();
	DASSERT((m_size.x & (m_size.x - 1)) == 0 && m_size.x >= 8);
	m_line_size = (m_size.x + 7) / 8;
	m_bitmap.resize(m_line_size * m_size.y, 0);

	vector<u8> height_map(m_size.x * m_size.y);
	extractheightMap(tile_map, &height_map[0], m_size, 2);

	for(int z = 0; z < m_size.y; z++) {
		const u8 *src = &height_map[z * m_size.x];
		u8 *dst = &m_bitmap[z * m_line_size];

		for(int x = 0; x < m_size.x; x += 8) {
			u8 octet = 0;
			for(int i = 0; i < 8; i++)
				octet |= src[x + i] == 1? (1 << i) : 0;
			dst[x >> 3] = octet;
		}
	}
}

gfx::PTexture NavigationBitmap::getTexture() const {
	gfx::Texture tex(size().x, size().y);

	for(int y = 0; y < m_size.y; y++)
		for(int x = 0; x < m_size.x; x++)
			tex(x, y) = (*this)(x, y)? Color(255, 255, 255) : Color(0, 0, 0);
	
	gfx::PTexture out = new gfx::DTexture;
	out->setSurface(tex);
	return out;
}

void NavigationBitmap::printInfo() const {
	printf("NavigationBitmap(%d, %d):\n", m_size.x, m_size.y);
	printf("  bitmap: %.0f KB\n", double(m_bitmap.size()) / 1024.0);
}

