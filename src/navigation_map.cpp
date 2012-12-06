#include "navigation_map.h"
#include "navigation_bitmap.h"
#include "gfx/texture.h"
#include <cstring>
#include <cmath>
#include <set>
#include <algorithm>

NavigationMap::NavigationMap(int extend) :m_size(0, 0), m_extend(extend) { }

enum { sector_size = 1024 };

	
static IRect findBestRect(const short *counts, const short *skip_list, int2 size) __attribute__((noinline));
static IRect findBestRect(const short *counts, const short *skip_list, int2 size) {
	IRect best;
	int best_score = -1;

	for(int sy = 0; sy < size.y; sy++) {
		struct Element { int sx, height; } stack[sector_size];
		int sp = 0;

		for(int sx = 0; sx <= size.x;) {
			int offset = sx + sy * sector_size;
			int height = sx == size.x? 0 : counts[offset];
			int min_sx = sx;

			while(sp && stack[sp - 1].height > height) {
				sp--;

				int w = sx - stack[sp].sx, h = stack[sp].height;
				int tmin = min(w, h);
				int score = w * h + tmin * tmin * 4;

				if(score > best_score) {
					best = IRect(stack[sp].sx, sy - stack[sp].height + 1, sx, sy + 1);
					best_score = score;
				}
				min_sx = min(min_sx, stack[sp].sx);
			}

			if(height)
				stack[sp++] = Element{min_sx, height};

			sx += height || sx == size.x? 1 : skip_list[offset];
		}
	}

	return best;
}

void NavigationMap::extractQuads(const NavigationBitmap &bitmap, int sx, int sy) {
	int2 size(min((int)sector_size, m_size.x - sx), min((int)sector_size, m_size.y - sy));

	int pixels = 0;
	short *counts = new short[sector_size * size.y * 2];
	short *skips = counts + sector_size * size.y;

	for(int y = 0; y < size.y; y++) {
		int yoff = y * sector_size;
		for(int x = 0; x < size.x; x++) {
			if(bitmap(sx + x, sy + y)) {
				counts[x + yoff] = 1 + (y > 0? counts[x + yoff - sector_size] : 0);
				pixels++;
			}
			else
				counts[x + yoff] = 0;
		}

		int prev = size.x;
		for(int x = size.x - 1; x >= 0; x--) {
			if(counts[x + yoff])
				prev = x;
			skips[x + yoff] = max(1, prev - x);
		}
	}

	while(pixels > 0) {
		IRect best = findBestRect(&counts[0], &skips[0], size);
	//	printf("%d %d %d %d +(%d %d)\n", best.min.x, best.min.y, best.width(), best.height(), sx, sy);

		for(int y = best.min.y; y < best.max.y; y++) {
			int yoff = y * sector_size;
			for(int x = best.min.x; x < best.max.x; x++)
				counts[x + yoff] = 0;
			int next = best.max.x;
			if(next < size.x && !counts[next + yoff])
				next += skips[next + yoff];
			for(int x = best.min.x; x < best.max.x; x++)
				skips[x + yoff] = next - x;
		}
		
		for(int x = best.min.x; x < best.max.x; x++) {
			for(int y = best.max.y; y < size.y; y++) {
				int offset = x + y * sector_size;
				counts[offset] = counts[offset]? counts[offset - sector_size] + 1 : 0;
				if(!counts[offset])
					break;
			}
		}
				
		m_quads.push_back(Quad(best + int2(sx, sy)));

		pixels -= best.width() * best.height();
	}

	delete[] counts;
}

void NavigationMap::addAdjacencyInfo(int quad1_id, int quad2_id) {
	DASSERT(quad1_id != quad2_id);
	Quad &quad1 = m_quads[quad1_id];
	Quad &quad2 = m_quads[quad2_id];

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

		quad1.neighbours.push_back(quad2_id);
		quad1.edges.push_back(edge);
	}
}

void NavigationMap::update(const NavigationBitmap &bitmap) {
	m_size = bitmap.size();
	m_quads.clear();
	DASSERT(bitmap.extend() == m_extend);

	printf("Creating navigation map: "); fflush(stdout);
	double time = getTime();
	for(int sy = 0; sy < m_size.y; sy += sector_size)
		for(int sx = 0; sx < m_size.x; sx += sector_size) {
			extractQuads(bitmap, sx, sy);
			//printf("."); fflush(stdout);
		}
	printf("%.2f seconds\n", getTime() - time);
	m_static_count = (int)m_quads.size();

	for(int i = 0; i < (int)m_quads.size(); i++)
		for(int j = 0; j < (int)m_quads.size(); j++)
			if(j != i)
				addAdjacencyInfo(i, j);
	for(int n = 0; n < (int)m_quads.size(); n++)
		m_quads[n].static_ncount = (int)m_quads[n].neighbours.size();
}

void NavigationMap::addCollider(int parent_id, const IRect &rect) {
	Quad &parent = m_quads[parent_id];
	IRect prect = parent.rect;
	IRect crect(max(rect.min, prect.min), min(rect.max, prect.max));
	if(crect.isEmpty())
		return;

	IRect rects[4];
	rects[0] = IRect(prect.min, int2(crect.max.x, crect.min.y));
	rects[1] = IRect(int2(crect.max.x, prect.min.y), int2(prect.max.x, crect.max.y));
	rects[2] = IRect(int2(prect.min.x, crect.min.y), int2(crect.min.x, prect.max.y));
	rects[3] = IRect(int2(crect.min.x, crect.max.y), prect.max);
	parent.is_disabled = true;

	int first_id = (int)m_quads.size();
	for(int n = 0; n < COUNTOF(rects); n++)
		if(!rects[n].isEmpty())
			m_quads.push_back(Quad(rects[n]));

	for(int n = first_id; n < (int)m_quads.size(); n++) {
		for(int i = 0; i < (int)parent.neighbours.size(); i++) {
			addAdjacencyInfo(n, i);
			addAdjacencyInfo(i, n);
		}
		for(int i = 0; i < n; i++) {
			addAdjacencyInfo(n, i);
			addAdjacencyInfo(i, n);
		}
	}
}

void NavigationMap::addCollider(const IRect &rect) {
	if(rect.isEmpty())
		return;

	IRect extended_rect(rect.min - int2(m_extend, m_extend), rect.max);

	for(int n = 0, count = (int)m_quads.size(); n < count; n++)
		if(!m_quads[n].is_disabled && areOverlapping(m_quads[n].rect, extended_rect))
			addCollider(n, extended_rect);
}

void NavigationMap::removeColliders() {
	m_quads.resize(m_static_count);
	for(int n = 0; n < (int)m_quads.size(); n++) {
		Quad &quad = m_quads[n];
		quad.neighbours.resize(quad.static_ncount);
		quad.edges.resize(quad.static_ncount);
		quad.is_disabled = false;
	}
}

static float distance(const int2 &a, const int2 &b) {
	int dist_x = abs(a.x - b.x), dist_y = abs(a.y - b.y);
	int dist_diag = min(dist_x, dist_y);
	return float(dist_diag) * (1.414213562f - 2.0f) + float(dist_x + dist_y);
}

// Instead, maybe it would be better to search closest path with Rect as a target?
// (we would use dist_to rect extended by (-extend,-extend) and (1,1)
int2 NavigationMap::findClosestCorrectPos(const int2 &pos, const IRect &dist_to) const {
	int2 closest_pos = pos;
	float min_distance = 1.0f / 0.0f, min_distance2 = 0.0f;
	FRect fdist_to(dist_to.min, dist_to.max);

	for(int n = 0; n < (int)m_quads.size(); n++) {
		int2 new_pos = clamp(pos, m_quads[n].rect.min, m_quads[n].rect.max - int2(1, 1));
		float dist = distanceSq(fdist_to, FRect(new_pos, new_pos + int2(3, 3)));
		float dist2 = distanceSq(new_pos, pos);

		if(dist < min_distance || (dist == min_distance && dist2 < min_distance2)) {
			closest_pos = new_pos;
			min_distance = dist;
			min_distance2 = dist2;
		}
	}

	return closest_pos;
}

int NavigationMap::findQuad(int2 pos, bool find_disabled) const {
	//TODO: speed up?
	for(int n = 0; n < (int)m_quads.size(); n++)
		if(m_quads[n].is_disabled == find_disabled && m_quads[n].rect.isInside(pos))
			return n;
	return -1;
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

			if(next.is_finished || next.is_disabled)
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
		if(input[n].quad_id != input[n + 1].quad_id && vec.ddiag &&
				(!(prev_diag || prev_dx != !!vec.dx) || (vec.dx == 0 && vec.dy == 0))) {
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
		if(m_quads[n].is_disabled)
			continue;
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

void NavigationMap::printInfo() const {
	printf("NavigationMap(%d, %d):\n", m_size.x, m_size.y);

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
