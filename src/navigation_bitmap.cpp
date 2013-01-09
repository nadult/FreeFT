#include "navigation_bitmap.h"
#include "grid.h"
#include "gfx/device.h"

static void extractheightMap(const Grid &tile_grid, u8 *out, int2 size, int extend) {
	DASSERT(out);
	memset(out, 0, size.x * size.y);
	IBox dbox(int3(0, 0, 0), int3(size.x, 255, size.y));
	
	for(int n = 0; n < tile_grid.size(); n++) {
		IBox bbox = (IBox)tile_grid[n].bbox;
			
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

void NavigationBitmap::blit(const IRect &rect, bool value) {
	IRect clipped(max(rect.min - int2(m_extend, m_extend), int2(0, 0)), min(rect.max, m_size));

	for(int y = clipped.min.y; y < clipped.max.y; y++)
		for(int x = clipped.min.x; x < clipped.max.x; x++) {
			int offset = (x >> 3) + y * m_line_size;
			int bit = 1 << (x & 7);

			if(value)
				m_bitmap[offset] |= bit;
			else
				m_bitmap[offset] &= ~bit;
		}
}

NavigationBitmap::NavigationBitmap(const Grid &tile_grid, int extend) :m_size(0, 0), m_extend(extend) {
	m_size = tile_grid.dimensions();
	m_line_size = (m_size.x + 7) / 8;
	m_bitmap.resize(m_line_size * m_size.y, 0);

	vector<u8> height_map(m_size.x * m_size.y);
	extractheightMap(tile_grid, &height_map[0], m_size, m_extend);

	for(int z = 0; z < m_size.y; z++) {
		const u8 *src = &height_map[z * m_size.x];
		u8 *dst = &m_bitmap[z * m_line_size];

		for(int x = 0; x < m_size.x; x += 8) {
			u8 octet = 0;
			for(int i = 0; i < 8; i++)
				octet |= src[x + i] >= 127 && src[x + i] <= 129? (1 << i) : 0;
			dst[x >> 3] = octet;
		}
	}
}

gfx::PTexture NavigationBitmap::getTexture() const {
	gfx::Texture tex(dimensions().x, dimensions().y);

	for(int y = 0; y < m_size.y; y++)
		for(int x = 0; x < m_size.x; x++)
			tex(x, y) = (*this)(x, y)? Color(255, 255, 255) : Color(0, 0, 0);
	
	gfx::PTexture out = new gfx::DTexture;
	out->set(tex);
	return out;
}

void NavigationBitmap::printInfo() const {
	printf("NavigationBitmap(%d, %d):\n", m_size.x, m_size.y);
	printf("  bitmap: %.0f KB\n", double(m_bitmap.size()) / 1024.0);
}

