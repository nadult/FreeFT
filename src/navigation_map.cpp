#include "navigation_map.h"
#include "gfx/texture.h"
#include <cstring>
#include <cmath>
#include <set>
#include <algorithm>

NavigationMap::NavigationMap(int2 size) :m_size(0, 0) {
	resize(size);
}

void NavigationMap::resize(int2 size) {
	DASSERT((size.x & (size.x - 1)) == 0 && size.x >= 8);
	m_line_size = (size.x + 7) / 8;
	m_bitmap.resize(m_line_size * size.y, 0);
	m_size = size;
}

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

void NavigationMap::update(const TileMap &tile_map) {
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

	extractQuads();
}

static bool areAdjacent(const IRect &a, const IRect &b) {
	if(b.min.x < a.max.x && a.min.x < b.max.x)
		return a.max.y == b.min.y || a.min.y == b.max.y;
	if(b.min.y < a.max.y && a.min.y < b.max.y)
		return a.max.x == b.min.x || a.min.x == b.max.x;
	return false;
}

void NavigationMap::extractQuads() {
	m_quads.clear();
	vector<int> line(m_size.x), line_min(m_size.x);
	vector<u8> bitmap_copy = m_bitmap;

	printf("Creating navigation map: "); fflush(stdout);
	int pixels = 0;
	for(int y = 0; y < m_size.y; y++)
		for(int x = 0; x < m_size.x; x++)
			if((*this)(x, y))
				pixels++;

	while(pixels) {
		vector<int> counts(m_size.x * m_size.y);
		for(int x = 0; x < m_size.x; x++)
			counts[x] = (*this)(x, 0)? 1 : 0;

		for(int y = 1; y < m_size.y; y++)
			for(int x = 0; x < m_size.x; x++) {
				int offset = x + y * m_size.x;
				counts[offset] = (*this)(x, y)? counts[offset - m_size.x] + 1 : 0;
			}
		
		IRect best;
		float best_score = -1;

		for(int sy = 0; sy < m_size.y; sy++) {
			for(int sx = 0; sx < m_size.x; sx++) {
				int height = counts[sx + sy * m_size.x];
				for(int x = sx; x < m_size.x; x++) {
					height = min(height, counts[x + sy * m_size.x]);
					if(height == 0)
						break;

					int width = x - sx + 1;
					float score = height * width;
					float diff = min(width, height) / float(max(width, height));
					score = score * (0.5f + diff);

					if(score > best_score) {
						best = IRect(sx, sy - height + 1, x + 1, sy + 1);
						best_score = score;
					}
				}
			}
		}

		for(int y = best.min.y; y < best.max.y; y++)
			for(int x = best.min.x; x < best.max.x; x++)
				m_bitmap[(x >> 3) + y * m_line_size] &= ~(1 << (x & 7));
				
		Quad new_quad;
		new_quad.rect = best;
		m_quads.push_back(new_quad);

		pixels -= best.width() * best.height();
		printf("."); fflush(stdout);
	//	printf("%d %d %d %d (%d)\n", best.min.x, best.min.y, best.width(), best.height(), pixels);
	}
	bitmap_copy.swap(m_bitmap);
	printf("\n");

	for(int i = 0; i < (int)m_quads.size(); i++) {
		Quad &quad1 = m_quads[i];
		for(int j = i + 1; j < (int)m_quads.size(); j++) {
			Quad &quad2 = m_quads[j];
			if(areAdjacent(quad1.rect, quad2.rect)) {
				IRect edge;
				edge.min = max(quad1.rect.min, quad2.rect.min);
				edge.max = min(quad1.rect.max, quad2.rect.max);

				quad1.neighbours.push_back(j);
				quad2.neighbours.push_back(i);
				quad1.edges.push_back(edge);
				quad2.edges.push_back(edge);
			}
		}
	}
}

int NavigationMap::findQuad(int2 pos) const {
	for(int n = 0; n < (int)m_quads.size(); n++)
		if(m_quads[n].rect.isInside(pos))
			return n;
	return -1;
}

static int distance(const int2 &a, const int2 &b) {
	int dist_x = abs(a.x - b.x), dist_y = abs(a.y - b.y);
	int dist_min = min(dist_x, dist_y);
	return (dist_min * 181) / 128 + (dist_x + dist_y - dist_min * 2);
}

// Liczymy odległości pomiędzy krawędziami, 
// zapisujemy najbliższy punkt leżący na krawędzi i odległość do niego
//
// dla każdej krawędzi trzymamy listę potencjalnych punktów
// przy dodawaniu nowego punktu P sprawdzamy czy odległość punktu Q z krawędzi + dist(Q, P) jest <=
// odległości punktu P; jeśli tak to P odrzucamy

vector<int2> NavigationMap::findPath(int2 start, int2 end) const {
	vector<int2> out;
	int start_id = findQuad(start), end_id = findQuad(end);

	if(start_id == -1 || end_id == -1) //TODO: info that path not found
		return out;

	for(int n = 0; n < (int)m_quads.size(); n++) {
		m_quads[n].dist = -1;
		m_quads[n].is_finished = false;
	}

/*	vector<pair<int, int> > edge_ids;
	for(int n = 0; n < (int)m_quads.size(); n++)
		for(int i = 0; i < (int)m_quads[n].edges.size(); i++)
			edge_ids.push_back(make_pair(n, i));
	printf("edge count: %d\n", (int)edge_ids.size());

	struct Point {
		int2 pos;
		int dist;
	};

	vector<vector<Point>> edges_points(edge_ids.size());
	int epoints = 0;
	for(int n = 0; n < edge_ids.size(); n++) {
		IRect box = m_quads[edge_ids[n].first].edges[edge_ids[n].second];
		epoints += box.max.y + box.max.x - box.min.x - box.min.y;
	}
	printf("ep: %d\n", epoints); */

	m_quads[start_id].dist = 0;
	m_quads[start_id].entry_pos = start;
	m_quads[start_id].src_quad = -1;

	std::set<pair<int, int> > queue;
	queue.insert(make_pair(0, start_id));

	while(!queue.empty()) {
		int quad_id = queue.begin()->second;
		if(quad_id == end_id)
			break;

		const Quad &quad = m_quads[quad_id];
		quad.is_finished = true;
		queue.erase(queue.begin());

		for(int n = 0; n < (int)quad.neighbours.size(); n++) {
			int next_id = quad.neighbours[n];
			const Quad &next = m_quads[next_id];

			int2 closest_pos = clamp(quad.entry_pos, next.rect.min, next.rect.max - int2(1, 1));
			closest_pos = clamp(closest_pos, quad.rect.min - int2(1, 1), quad.rect.max);
			int dist = distance(closest_pos, quad.entry_pos) + quad.dist;

			if(next_id == end_id)
				dist += distance(closest_pos, end);

			if(next.is_finished || (next.dist != -1 && next.dist <= dist))
				continue;

			if(next.dist != -1)
				queue.erase(make_pair(next.dist, next_id));

			next.dist = dist;
			next.entry_pos = closest_pos;
			next.src_quad = quad_id;
			queue.insert(make_pair(dist, next_id));
		}
	}

	out.push_back(end);
	int quad_id = end_id, prev_id = -1;
	bool prev_diag = false;

	while(quad_id != -1) {
		const Quad &quad = m_quads[quad_id];
		if(out.back() != quad.entry_pos)
			out.push_back(quad.entry_pos);
		quad_id = quad.src_quad;
	}
	std::reverse(out.begin(), out.end());

	return out;
}

void NavigationMap::visualize(gfx::SceneRenderer &renderer, bool borders) const {
	for(int n = 0; n < (int)m_quads.size(); n++) {
		const IRect &rect = m_quads[n].rect;

		renderer.addBox(IBox(asXZY(rect.min, 1), asXZY(rect.max, 1)), m_quads[n].color, true);
		if(borders)
			renderer.addBox(IBox(asXZY(rect.min, 1), asXZY(rect.max, 1)), Color(255, 255, 255, 100));
	}
}

gfx::PTexture NavigationMap::getTexture() const {
	gfx::Texture tex(size().x, size().y);

	for(int y = 0; y < m_size.y; y++)
		for(int x = 0; x < m_size.x; x++)
			tex(x, y) = (*this)(x, y)? Color(255, 255, 255) : Color(0, 0, 0);
	
	gfx::PTexture out = new gfx::DTexture;
	out->setSurface(tex);
	return out;
}

void NavigationMap::printInfo() const {
	printf("NavigationMap(%d, %d):\n", m_size.x, m_size.y);
	printf("  bitmap: %.0f KB\n", double(m_bitmap.size()) / 1024.0);

	int bytes = sizeof(Quad) * m_quads.size();
	for(int n = 0; n < (int)m_quads.size(); n++) {
		bytes += m_quads[n].neighbours.size() * sizeof(int);
		bytes += m_quads[n].edges.size() * sizeof(IRect);
	}
	printf("  quads(%d): %.0f KB\n", (int)m_quads.size(), double(bytes) / 1024.0);

	if(0) for(int n = 0; n < (int)m_quads.size(); n++) {
		const Quad &quad = m_quads[n];
		printf("%d: (%d %d %d %d): ", n, quad.rect.min.x, quad.rect.min.y, quad.rect.max.x, quad.rect.max.y);
		for(int i = 0; i < (int)quad.neighbours.size(); i++)
			printf("%d ", quad.neighbours[i]);
		printf("\n");
	}
}

