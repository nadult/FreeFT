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
	
static IRect findBestRect(short *counts, int *line_counts, int stride, int2 size) {
	IRect best;
	int best_score = -1;

	for(int sy = 0; sy < size.y; sy++) {
		pair<int, int> stack[size.x];
		int sp = 0;

		if(!line_counts[sy])
			continue;

		for(int sx = 0; sx <= size.x; sx++) {
			int height = sx == size.x? 0 : counts[sx + sy * stride];
			int min_k = sx;

			for(int i = 0; i < sp; i++)
				if(stack[i].second > height) {
					int w = sx - stack[i].first, h = stack[i].second;
					int tmin = min(w, h);
					int score = w * h + tmin * tmin * 4;

					if(score > best_score) {
						best = IRect(stack[i].first, sy - stack[i].second + 1, sx, sy + 1);
						best_score = score;
					}
					min_k = min(min_k, stack[i].first);
					stack[i--] = stack[--sp];
				}

			if(height)
				stack[sp++] = make_pair(min_k, height);
		}
	}

	return best;
}

	
enum { sector_size = 256 };

void NavigationMap::extractQuads(int sx, int sy) {
	int2 size(min((int)sector_size, m_size.x - sx), min((int)sector_size, m_size.y - sy));

	int pixels = 0;
	vector<u8> tbitmap(sector_size * sector_size, 0);
	vector<short> counts(sector_size * sector_size, 0);
	int line_counts[size.y];

	for(int y = 0; y < size.y; y++) {
		int line_count = 0;
		for(int x = 0; x < size.x; x++)
			if( (tbitmap[x + y * sector_size] = (*this)(sx + x, sy + y)) )
				line_count++;
		pixels += line_count;
		line_counts[y] = line_count;
	}

	for(int x = 0; x < size.x; x++)
		counts[x] = tbitmap[x]? 1 : 0;

	for(int y = 1; y < size.y; y++)
		for(int x = 0; x < size.x; x++) {
			int offset = x + y * sector_size;
			counts[offset] = tbitmap[offset]? counts[offset - sector_size] + 1 : 0;
		}


	while(pixels > 0) {
		IRect best = findBestRect(&counts[0], line_counts, sector_size, int2(sector_size, sector_size));
	//	printf("%d %d %d %d +(%d %d)\n", best.min.x, best.min.y, best.width(), best.height(), sx, sy);

		for(int y = best.min.y; y < best.max.y; y++) {
			line_counts[y] -= best.max.x - best.min.x;
			for(int x = best.min.x; x < best.max.x; x++)
				tbitmap[x + y * sector_size] = 0;
		}
		
		for(int x = best.min.x; x < best.max.x; x++) {
			for(int y = best.min.y; y < best.max.y; y++) {
				int offset = x + y * sector_size;
				counts[offset] = 0;
			}

			for(int y = best.max.y; y < size.y; y++) {
				int offset = x + y * sector_size;
				counts[offset] = tbitmap[offset]? counts[offset - sector_size] + 1 : 0;
				if(!counts[offset])
					break;
			}
		}
				
		Quad new_quad;
		new_quad.rect = best + int2(sx, sy);
		m_quads.push_back(new_quad);

		pixels -= best.width() * best.height();
	}
}


void NavigationMap::extractQuads() {
	m_quads.clear();

	printf("Creating navigation map: "); fflush(stdout);
	double time = getTime();
	for(int sy = 0; sy < m_size.y; sy += sector_size)
		for(int sx = 0; sx < m_size.x; sx += sector_size) {
			extractQuads(sx, sy);
			printf("."); fflush(stdout);
		}
	printf("(%.2f seconds)\n", getTime() - time);

	for(int i = 0; i < (int)m_quads.size(); i++) {
		Quad &quad1 = m_quads[i];
		for(int j = 0; j < (int)m_quads.size(); j++) {
			if(j == i)
				continue;

			const Quad &quad2 = m_quads[j];
			if(areAdjacent(quad1.rect, quad2.rect)) {
				bool is_horizontal = quad1.rect.max.x > quad2.rect.min.x && quad1.rect.min.x < quad2.rect.max.x;
				IRect edge(max(quad1.rect.min, quad2.rect.min), min(quad1.rect.max, quad2.rect.max));

				if(is_horizontal)
					edge.max.x--;
				else
					edge.max.y--;
				
				if( is_horizontal && quad1.rect.min.y > quad2.rect.min.y)
					edge -= int2(0, 1);
				if(!is_horizontal && quad1.rect.min.x > quad2.rect.min.x)
					edge -= int2(1, 0);
				ASSERT(edge.max.x >= edge.min.x || edge.max.y >= edge.min.y);

				quad1.neighbours.push_back(j);
				quad1.edges.push_back(edge);
			}
		}
	}
}

int NavigationMap::findQuad(int2 pos) const {
	//TODO: speed up?
	for(int n = 0; n < (int)m_quads.size(); n++)
		if(m_quads[n].rect.isInside(pos))
			return n;
	return -1;
}

static float distance(const int2 &a, const int2 &b) {
	int dist_x = abs(a.x - b.x), dist_y = abs(a.y - b.y);
	int dist_diag = min(dist_x, dist_y);
	return float(dist_diag) * (1.414213562f - 2.0f) + float(dist_x + dist_y);
}

vector<NavigationMap::PathNode> NavigationMap::findPath(int2 start, int2 end, bool do_refining) const {
	vector<PathNode> out;
	int start_id = findQuad(start), end_id = findQuad(end);

	if(start_id == -1 || end_id == -1) //TODO: info that path not found
		return out;

	for(int n = 0; n < (int)m_quads.size(); n++) {
		m_quads[n].dist = 1.0f / 0.0f;
		m_quads[n].is_finished = false;
	}

	m_quads[start_id].dist = 0.0f;
	m_quads[start_id].edist = distance(start, end);
	m_quads[start_id].entry_pos = start;
	m_quads[start_id].src_quad = -1;

	std::set<pair<int, int> > queue;
	queue.insert(make_pair(m_quads[start_id].edist, start_id));

	bool end_reached = start_id == end_id;

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
			const IRect &edge = quad.edges[n];

			if(next.is_finished)
				continue;

			int2 edge_end_pos = clamp(end, edge.min, edge.max);
			MoveVector vec(quad.entry_pos, edge_end_pos);

			int2 closest_pos = clamp(quad.entry_pos + vec.vec * vec.ddiag, next.rect.min, next.rect.max - int2(1, 1));

			closest_pos = clamp(closest_pos, quad.rect.min - int2(1, 1), quad.rect.max);
	
			// fixing problem with diagonal moves through obstacle corners
			if(quad.rect.max.x > next.rect.min.x && quad.rect.min.x < next.rect.max.x) {
				if(quad.entry_pos.x < closest_pos.x && closest_pos.x == next.rect.min.x && closest_pos.x < next.rect.max.x - 1)
					closest_pos.x++;
				if(quad.entry_pos.x > closest_pos.x && closest_pos.x == next.rect.max.x - 1 && closest_pos.x > next.rect.min.x)
					closest_pos.x--;
			}
			else {
				if(quad.entry_pos.y < closest_pos.y && closest_pos.y == next.rect.min.y && closest_pos.y < next.rect.max.y - 1)
					closest_pos.y++;
				if(quad.entry_pos.y > closest_pos.y && closest_pos.y == next.rect.max.y - 1 && closest_pos.y > next.rect.min.y)
					closest_pos.y--;
			}

			float dist = distance(closest_pos, quad.entry_pos) + quad.dist;
			float edist = distance(closest_pos, end);

			if(next_id == end_id) {
				end_reached = true;
				dist += distance(closest_pos, end);
			}

			if(do_refining? next.dist <= dist : next.dist + next.edist <= dist + edist)
				continue;

			if(next.dist != -1)
				queue.erase(make_pair(next.dist + next.edist, next_id));

			next.dist = dist;
			next.edist = edist;
			next.entry_pos = closest_pos;
			next.src_quad = quad_id;
			queue.insert(make_pair(dist + next.edist, next_id));
		}
	}

	if(!end_reached)
		return out;

	out.push_back(PathNode{end, end_id});
	for(int quad_id = end_id; quad_id != -1; quad_id = m_quads[quad_id].src_quad) {
		const Quad &quad = m_quads[quad_id];
		if(out.back().point != quad.entry_pos)
			out.push_back({quad.entry_pos, quad_id});
	}
	std::reverse(out.begin(), out.end());
	
	if(do_refining) {
		for(int n = 0; n < (int)out.size() - 3; n++) {
			float dist =	distance(out[n + 0].point, out[n + 1].point) +
							distance(out[n + 1].point, out[n + 2].point) +
							distance(out[n + 2].point, out[n + 3].point);
			float sdist = distance(out[n].point, out[n + 3].point);
			if(sdist * 1.001f >= dist)
				continue;

			vector<PathNode> other = findPath(out[n].point, out[n + 3].point, false);

			if(!other.empty()) {
				ASSERT(other.front().point == out[n].point && other.back().point == out[n + 3].point);

				float odist = 0;
				for(int i = 0; i < (int)other.size() - 1; i++)
					odist += distance(other[i].point, other[i + 1].point);
				if(odist + 0.01 < dist) {
					//printf("refining... %f -> %f\n", dist, odist);

					out.erase(out.begin() + n + 1);
					out.erase(out.begin() + n + 1);
					out.insert(out.begin() + n + 1, other.begin() + 1, other.begin() + other.size() - 1);
					n--;
				}
			}
		}
	}

	return out;
}

vector<int2> NavigationMap::findPath(int2 start, int2 end) const {
	vector<PathNode> input = findPath(start, end, true);
	vector<int2> path;

	if(input.empty())
		return path;

	for(int n = 0; n < (int)input.size() - 1; n++) {
		const IRect &src_quad = m_quads[input[n + 0].quad_id].rect;
		const IRect &dst_quad = m_quads[input[n + 1].quad_id].rect;

		int2 src = input[n + 0].point;
		int2 dst = input[n + 1].point;
		MoveVector vec(src, dst);
		MoveVector prev_vec = path.empty()? MoveVector() : MoveVector(path.back(), src);
		
		bool is_horizontal = src_quad.max.x > dst_quad.min.x && src_quad.min.x < dst_quad.max.x;

		bool prev_diag = prev_vec.ddiag != 0;
		bool prev_dx = prev_vec.dx != 0;

		int2 pdst = dst;
		if(input[n].quad_id != input[n + 1].quad_id && vec.ddiag && (!(prev_diag || prev_dx != !!vec.dx) || (vec.dx == 0 && vec.dy == 0))) {
			if(src.x < pdst.x) pdst.x--; else if(src.x > pdst.x) pdst.x++;
			if(src.y < pdst.y) pdst.y--; else if(src.y > pdst.y) pdst.y++;
		}
		pdst = clamp(pdst, src_quad.min, src_quad.max - int2(1,1));

		if(is_horizontal)
				pdst.x = clamp(pdst.x, dst_quad.min.x, dst_quad.max.x - 1);
		else
				pdst.y = clamp(pdst.y, dst_quad.min.y, dst_quad.max.y - 1);

		vec = MoveVector(src, pdst);
		int2 mid = src;

		if((prev_diag || prev_dx != !!vec.dx) && vec.ddiag) {
			mid += vec.vec * vec.ddiag;
			prev_diag = vec.dx == 0 && vec.dy == 0;
		}
		else {
			mid += vec.dx? int2(vec.vec.x * vec.dx, 0) : int2(0, vec.vec.y * vec.dy);
			vec.dx = vec.dy = 0;
			prev_diag = vec.ddiag != 0;
		}

		path.push_back(src);
		path.push_back(mid);
		path.push_back(pdst);
	}
	path.push_back(input.back().point);
	if(path.size() <= 2)
		return std::move(path);

	vector<int2> simplified;
	simplified.push_back(path[0]);
	int2 prev(0, 0);

	for(int n = 1; n < (int)path.size(); n++) {
		int2 vec = MoveVector(simplified.back(), path[n]).vec;
		if(vec == int2(0, 0))
			continue;

		if(vec == prev)
			simplified.back() = path[n];
		else {
			prev = vec;
			simplified.push_back(path[n]);
		}
	}

	return std::move(simplified);
}

void NavigationMap::visualize(gfx::SceneRenderer &renderer, bool borders) const {
	for(int n = 0; n < (int)m_quads.size(); n++) {
		const IRect &rect = m_quads[n].rect;

		renderer.addBox(IBox(asXZY(rect.min, 1), asXZY(rect.max, 1)), Color(70, 220, 200, 80), true);
		if(borders)
			renderer.addBox(IBox(asXZY(rect.min, 1), asXZY(rect.max, 1)), Color(255, 255, 255, 100));
	}
}

void NavigationMap::visualizePath(const vector<int2> &path, int elem_size, gfx::SceneRenderer &renderer) const {
	if(path.empty())
		return;

	IBox box(0, 1, 0, elem_size, 1, elem_size);
	renderer.addBox(box + asXZ(path.front()), Color::red);

	for(int n = 1; n < (int)path.size(); n++) {
		int2 begin = path[n - 1], end = path[n];
		bool first = true;

		IBox start_box = box + asXZ(begin), end_box = box + asXZ(end);
		renderer.addBox(end_box, Color::red);
		MoveVector vec(begin, end);

		if(vec.vec == int2(1, 1) || vec.vec == int2(-1, -1)) {
			swap(start_box.min.x, start_box.max.x);
			swap(  end_box.min.x,   end_box.max.x);
		}

		renderer.addLine(start_box.min, end_box.min);
		renderer.addLine(start_box.max, end_box.max);
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

