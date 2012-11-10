#include "collision_map.h"
#include "gfx/texture.h"
#include <cstring>
#include <cmath>

CollisionMap::CollisionMap(int2 size) :m_size(0, 0) {
	resize(size);
}

void CollisionMap::resize(int2 size) {
	DAssert((size.x & (size.x - 1)) == 0 && size.x >= 8);
	m_line_size = (size.x + 7) / 8;
	m_bitmap.resize(m_line_size * size.y, 0);
	m_size = size;
}

static void extractHeightMap(const TileMap &tile_map, u8 *out, int2 size, int extend) {
	DAssert(out);
	memset(out, 0, size.x * size.y);
	IBox dbox(int3(0, 0, 0), int3(size.x, 255, size.y));
	
	for(int t = 0; t < tile_map.nodeCount(); t++) {
		const TileMapNode &node = tile_map(t);
		int3 tnode_pos = tile_map.nodePos(t);
		
		for(int i = 0; i < node.instanceCount(); i++) {
			const TileInstance &inst = node(i);
			IBox bbox = inst.boundingBox() + tnode_pos;
			
			if(Overlaps(dbox, bbox)) {
				bbox.min = Max(bbox.min - int3(extend, 0, extend), int3(0, 0, 0));
				bbox.max = Min(bbox.max, dbox.max);

				u8 *ptr = out + bbox.min.x + bbox.min.z * size.x;
				for(int z = 0; z < bbox.Depth(); z++) {
					for(int x = 0; x < bbox.Width(); x++)
						ptr[x] = Max(ptr[x], (u8)bbox.max.y);
					ptr += size.x;
				}
			}
		}
	}
}

void CollisionMap::update(const TileMap &tile_map) {
	vector<u8> height_map(m_size.x * m_size.y);
	extractHeightMap(tile_map, &height_map[0], m_size, 3);

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

static bool areAdjacent(const IRect &a, const IRect &b) {
	if(b.min.x < a.max.x && a.min.x < b.max.x)
		return a.max.y == b.min.y || a.min.y == b.max.y;
	if(b.min.y < a.max.y && a.min.y < b.max.y)
		return a.max.x == b.min.x || a.min.x == b.max.x;
	return false;
}

vector<IRect> CollisionMap::extractQuads() {
	vector<IRect> out;
//	out.push_back(IRect(int2(0, 0), m_size));
	vector<int> line(m_size.x), line_min(m_size.x);

	for(int sz = 0; sz < m_size.y; sz++)
		for(int sx = 0; sx < m_size.x; sx++) {
			if(!(*this)(sx, sz))
				continue;

			int max_x = m_size.x;
			for(int x = sx; x < m_size.x; x++) {
				int z = sz;
				while(z < m_size.y && (*this)(x, z))
					z++;
				line[x] = z - sz;
				if(!line[x]) {
					max_x = x;
					break;
				}
			}

			IRect best;
			int best_score = -1;

			for(int sx2 = sx; sx2 < max_x; sx2++) {
				int min = line[sx2], waste = 0;
				for(int x = sx2; x < max_x; x++) {
					min = Min(min, line[x]);
					waste += Max(line[x] - min, min - line[x]);

					IRect rect(sx2, sz, x + 1, sz + min);
					int score = Min(x - sx2 + 1, min); score = score * score;// - waste;

					if(score > best_score || (score == best_score && rect.SurfaceArea() > best.SurfaceArea())) {
						best_score = score;
						best = rect;
					}
				}
			}

			for(int z = best.min.y; z < best.max.y; z++)
				for(int x = best.min.x; x < best.max.x; x++)
					m_bitmap[(x >> 3) + z * m_line_size] &= ~(1 << (x & 7));
		//	printf("%d %d %d %d\n", best.min.x, best.min.y, best.Width(), best.Height());

			out.push_back(best);
			sx = -1;
		}
	printf("Generated %d quads\n", (int)out.size());

	return out;
}

gfx::PTexture CollisionMap::getTexture() const {
	gfx::Texture tex(size().x, size().y);

	for(int y = 0; y < m_size.y; y++)
		for(int x = 0; x < m_size.x; x++)
			tex(x, y) = (*this)(x, y)? Color(255, 255, 255) : Color(0, 0, 0);
	
	gfx::PTexture out = new gfx::DTexture;
	out->SetSurface(tex);
	return out;
}

void CollisionMap::printInfo() const {
//	printf("CollisionMap(%d, %d): %d x %d nodes (%dx%d in size)\n",
//			m_size.x * node_size, m_size.y * node_size, m_size.x, m_size.y, node_size, node_size);
//	printf("    Node[%d] (%d bytes): %.0f KB\n", (int)m_nodes.size(), (int)sizeof(Node),
//			double(m_nodes.size() * sizeof(Node)) / double(1024));
//	printf("NodeData[%d] (%d bytes): %.0f KB\n", (int)m_nodes_data.size(), (int)sizeof(NodeData),
//			double(m_nodes_data.size() * sizeof(NodeData)) / double(1024));

}

