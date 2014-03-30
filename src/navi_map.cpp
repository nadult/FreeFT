/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "navi_map.h"
#include "navi_heightmap.h"
#include "sys/profiler.h"
#include "gfx/scene_renderer.h"
#include <cstring>
#include <algorithm>

NaviMap::NaviMap(int extend) :m_size(0, 0), m_agent_size(extend) { }

	
static IRect findBestRect(const short *counts, const short *skip_list, int2 size) __attribute__((noinline));
static IRect findBestRect(const short *counts, const short *skip_list, int2 size) {
	IRect best;
	int best_score = -1;

	for(int sy = 0; sy < size.y; sy++) {
		struct Element { int sx, height; } stack[NaviMap::sector_size];
		int sp = 0;

		for(int sx = 0; sx <= size.x;) {
			int offset = sx + sy * NaviMap::sector_size;
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

void NaviMap::extractQuads(const PodArray<u8> &bitmap, int sx, int sy) {
	int2 size(min((int)sector_size, m_size.x - sx), min((int)sector_size, m_size.y - sy));

	int pixels = 0;
	PodArray<short> counts(sector_size * size.y * 2);
	short *skips = counts.data() + sector_size * size.y;

	for(int y = 0; y < size.y; y++) {
		int yoff = y * sector_size;
		for(int x = 0; x < size.x; x++) {
			if(bitmap[sx + x + (sy + y) * m_size.x]) {
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
		
		IRect rect = best + int2(sx, sy);
		int min_height = bitmap[rect.min.x + rect.min.y * m_size.x];
		int max_height = min_height;

		for(int y = rect.min.y; y < rect.max.y; y++)
			for(int x = rect.min.x; x < rect.max.x; x++) {
				int height = bitmap[x + y * m_size.x];
				min_height = min(min_height, height);
				max_height = max(max_height, height);
			}

		m_quads.push_back(Quad(rect, min_height, max_height));

		pixels -= best.width() * best.height();
	}
}

static const IRect computeEdge(const IRect &quad1, const IRect &quad2) {
	bool is_horizontal = quad1.max.x > quad2.min.x && quad1.min.x < quad2.max.x;
	IRect edge(max(quad1.min, quad2.min), min(quad1.max, quad2.max));

	if(is_horizontal) {
		edge.max.x--;
		if(quad1.min.y > quad2.min.y) {
			edge.min.y--;
			edge.max.y--;
		}
	}
	else {
		edge.max.y--;
		if(quad1.min.x > quad2.min.x) {
			edge.min.x--;
			edge.max.x--;
		}
	}

	DASSERT(edge.max.x >= edge.min.x || edge.max.y >= edge.min.y);
	return edge;
}

void NaviMap::addAdjacencyInfo(int quad1_id, int quad2_id) {
	DASSERT(quad1_id != quad2_id);
	Quad &quad1 = m_quads[quad1_id];
	Quad &quad2 = m_quads[quad2_id];

	if(quad1.min_height <= quad2.max_height + 1 && quad2.min_height <= quad1.max_height + 1 &&
			areAdjacent(quad1.rect, quad2.rect)) {
		quad1.neighbours.push_back(quad2_id);
		quad2.neighbours.push_back(quad1_id);
	}
}

void NaviMap::update(const NaviHeightmap &heightmap) {
	m_size = heightmap.dimensions();
	m_quads.clear();

	printf("Creating navigation map: "); fflush(stdout);
	double time = getTime();

	for(int l = 0; l < heightmap.levelCount(); l++) {
		PodArray<u8> bitmap(m_size.x * m_size.y);
		memset(bitmap.data(), 0, bitmap.dataSize());
		int pixel_count = 0;

		for(int y = 0; y < m_size.y; y++)
			for(int x = 0; x < m_size.x; x++)
				if(heightmap.test(x, y, l, m_agent_size)) {
					pixel_count++;
					bitmap[x + y * m_size.x] = heightmap(x, y, l);
				}
		PodArray<u8> subbitmap = bitmap;
		memset(subbitmap.data(), 0, subbitmap.dataSize());

		vector<int2> positions;
		positions.reserve((m_size.x + m_size.y) * 4);
		int max_diff = 0;
		int start_line = 0;

		while(pixel_count) {
			IRect subrect;
			positions.clear();
			int hmin, hmax;

			for(int y = start_line; y < m_size.y; y++) {
				for(int x = 0; x < m_size.x; x++) 
					if(bitmap[x + y * m_size.x]) {
						hmin = hmax = bitmap[x + y * m_size.x];
						positions.push_back(int2(x, y));
						subrect = IRect(x, y, x, y);
						break;
					}

				if(!positions.empty())
					break;
				start_line = y + 1;
			}

			while(!positions.empty()) {
				int2 pos = positions.back();
				positions.pop_back();

				int offset = pos.x + pos.y * m_size.x;
				int height = bitmap[offset];
				if(!height)
					continue;
				if(max(height - hmin, hmax - height) > max_diff)
					continue;

				hmin = min(hmin, height);
				hmax = max(hmax, height);

				subrect.min = min(subrect.min, pos);
				subrect.max = max(subrect.max, pos);
				bitmap[offset] = 0;
				subbitmap[offset] = height;
				pixel_count--;

				int2 offsets[4] = {
					int2(-1, 0),
					int2(0, -1),
					int2(1, 0),
					int2(0, 1)
				};

				int neighbours[4] = {
					pos.x > 0? bitmap[offset - 1] : 0,
					pos.y > 0? bitmap[offset - m_size.x] : 0,
					pos.x < m_size.x - 1? bitmap[offset + 1] : 0,
					pos.y < m_size.y - 1? bitmap[offset + m_size.x] : 0
				};

				for(int n = 0; n < 4; n++) {
					int height = neighbours[n];
					if(height && max(height - hmin, hmax - height) <= max_diff)
						positions.push_back(pos + offsets[n]);
				}
			}
			subrect.max += int2(1, 1);

			for(int sy = 0; sy < m_size.y; sy += sector_size)
				for(int sx = 0; sx < m_size.x; sx += sector_size)
					if(areOverlapping(subrect, IRect(sx, sy, sx + sector_size, sy + sector_size)))
						extractQuads(subbitmap, sx, sy);

			for(int y = subrect.min.y; y < subrect.max.y; y++)
				memset(subbitmap.data() + y * m_size.x + subrect.min.x, 0, subrect.width());
		}

		printf("."); fflush(stdout);
	}
	m_static_count = (int)m_quads.size();

	for(int i = 0; i < (int)m_quads.size(); i++)
		for(int j = 0; j < i; j++)
			addAdjacencyInfo(i, j);
	for(int n = 0; n < (int)m_quads.size(); n++)
		m_quads[n].static_ncount = (int)m_quads[n].neighbours.size();
	printf(" %.2f seconds (%d quads)\n", getTime() - time, (int)m_quads.size());
}

void NaviMap::addCollider(int parent_id, const IRect &rect) {
	Quad *parent = &m_quads[parent_id];
	IRect prect = parent->rect;
	IRect crect(max(rect.min, prect.min), min(rect.max, prect.max));
	if(crect.isEmpty())
		return;

	IRect rects[4];
	rects[0] = IRect(prect.min, int2(crect.max.x, crect.min.y));
	rects[1] = IRect(int2(crect.max.x, prect.min.y), int2(prect.max.x, crect.max.y));
	rects[2] = IRect(int2(prect.min.x, crect.min.y), int2(crect.min.x, prect.max.y));
	rects[3] = IRect(int2(crect.min.x, crect.max.y), prect.max);
	parent->is_disabled = true;

	int first_id = (int)m_quads.size();
	int min_height = parent->min_height;
	int max_height = parent->max_height;

	for(int n = 0; n < COUNTOF(rects); n++)
		if(!rects[n].isEmpty())
			m_quads.push_back(Quad(rects[n], min_height, max_height));
	parent = &m_quads[parent_id];

	int count = 0;
	for(int n = first_id; n < (int)m_quads.size(); n++) {
		for(int i = 0; i < (int)parent->neighbours.size(); i++)
			addAdjacencyInfo(n, parent->neighbours[i]);
		for(int i = first_id; i < n; i++)
			addAdjacencyInfo(n, i);
	}
}

void NaviMap::addCollider(const IRect &rect) {
	if(rect.isEmpty())
		return;

	//TODO
	IRect extended_rect(rect.min - int2(m_agent_size - 1, m_agent_size - 1), rect.max);

	for(int n = 0, count = (int)m_quads.size(); n < count; n++)
		if(!m_quads[n].is_disabled && areOverlapping(m_quads[n].rect, extended_rect))
			addCollider(n, extended_rect);
}

void NaviMap::removeColliders() {
	m_quads.resize(m_static_count);
	for(int n = 0; n < (int)m_quads.size(); n++) {
		Quad &quad = m_quads[n];
		quad.neighbours.resize(quad.static_ncount);
		quad.is_disabled = false;
	}
}

// TODO: Instead, maybe it would be better to search closest path with Box as a target?
// (we would use dist_to rect extended by (-extend,-extend) and (1,1)
int3 NaviMap::findClosestCorrectPos(const int3 &pos, const IBox &dist_to) const {
	int3 closest_pos = pos;
	float min_distance = 1.0f / 0.0f, min_distance2 = 0.0f;
	FRect fdist_to(dist_to.min.xz(), dist_to.max.xz());

	for(int n = 0; n < (int)m_quads.size(); n++) {
		if(m_quads[n].min_height > pos.y)
			continue;
		
		int3 new_pos = clamp(pos,	asXZY(m_quads[n].rect.min, m_quads[n].min_height),
								 	asXZY(m_quads[n].rect.max - int2(1, 1), m_quads[n].max_height));
		float dist  = distanceSq(fdist_to, FRect(new_pos.xz(), new_pos.xz() + int2(m_agent_size, m_agent_size)));
		float dist2 = distanceSq(new_pos, pos);

		if(dist < min_distance || (dist == min_distance && dist2 < min_distance2)) {
			closest_pos = new_pos;
			min_distance = dist;
			min_distance2 = dist2;
		}
	}

	return closest_pos;
}

int NaviMap::findQuad(const int3 &pos, bool find_disabled) const {
	int best_quad = -1;
	int best_height = -1;

	//TODO: speed up?
	for(int n = 0; n < (int)m_quads.size(); n++) {
		const Quad &quad = m_quads[n];

		if(quad.rect.isInside(pos.xz()) && quad.is_disabled == find_disabled
				&& pos.y >= quad.min_height && pos.y <= quad.max_height + 2.0f) {
			if(pos.y <= quad.max_height)
				return n;
			if(quad.max_height > best_height) {
				best_height = quad.max_height;
				best_quad = n;
			}
		}
	}

	return best_quad;
}

// powoduje problemy jak się przechodzi diagonalnie przez rogi encji
// (gracz się zatrzymuje bo jest kolizja)
// This should be solved by creating paths that are trying to be in the middle
// between colliders
//#define WALK_DIAGONAL_THROUGH_CORNERS


static float distance(const int2 &a, const int2 &b) {
	int dist_x = abs(a.x - b.x), dist_y = abs(a.y - b.y);
	int dist_diag = min(dist_x, dist_y);
	return float(dist_diag) * (1.414213562f - 2.0f) + float(dist_x + dist_y);
}

namespace {

	struct SearchData {
		int2 entry_pos;
		int src_quad;
		int heap_pos : 31;
		int is_finished : 1;
		float dist, est_dist;
	};

	struct HeapData {
		HeapData(SearchData *ptr) :ptr(ptr), value(ptr->dist + ptr->est_dist) { }
		HeapData() { }

		bool operator<(const HeapData &rhs) const { return value < rhs.value; }

		SearchData *ptr;
		float value;
	};

	bool heapInvariant(HeapData *heap, int size) {
		for(int n = 1; n < size; n++)
			if(heap[n] < heap[n / 2])
				return false;
		return true;
	}

	void printHeap(HeapData *heap, int size) {
		for(int n = 0; n < size; n++)
			printf("%.0f ", heap[n].value);
		printf("\n");
	}

#define UPDATE(id)	heap[id].ptr->heap_pos = id;
	void heapify(HeapData *heap, int size, int id) {
		int l = id * 2;
		int r = id * 2 + 1;

		int smallest = l < size && heap[l] < heap[id]? l : id;
		if(r < size && heap[r] < heap[smallest])
			smallest = r;
		if(smallest != id) {
			swap(heap[id], heap[smallest]);
			UPDATE(id);
			UPDATE(smallest);
			heapify(heap, size, smallest);
		}
	}

	HeapData extractMin(HeapData *heap, int &size) {
		DASSERT(size > 0);
		HeapData min = heap[0];

		heap[0] = heap[--size];
		UPDATE(0);
		heapify(heap, size, 0);
		return min;
	}

	void updateKey(HeapData *heap, int &size, SearchData *ptr) {
		int pos = ptr->heap_pos;
		DASSERT(pos == -1 || heap[pos].ptr == ptr);

		HeapData new_key(ptr);
		if(pos == -1)
			heap[pos = size++].value = new_key.value;

		if(new_key.value > heap[pos].value) {
			heap[pos].value = new_key.value;
			heapify(heap, size, pos);
			return;
		}

		while(pos > 0 && new_key < heap[pos / 2]) {
			heap[pos] = heap[pos / 2];
			UPDATE(pos);
			pos = pos / 2;
		}
		heap[pos] = new_key;
		UPDATE(pos);
	}

#undef UPDATE
}

vector<NaviMap::PathNode> NaviMap::findPath(const int2 &start, const int2 &end, int start_id, int end_id,
											bool do_refining) const {
	vector<PathNode> out;

	if(start_id == -1 || end_id == -1) //TODO: info that path not found
		return std::move(out);

	SearchData data[(int)m_quads.size()];
	for(int n = 0; n < (int)m_quads.size(); n++) {
		data[n].dist = 1.0f / 0.0f;
		data[n].heap_pos = -1;
		data[n].is_finished = false;
	}

	HeapData heap[(int)m_quads.size()];
	int heap_size = 0;

	data[start_id].dist = 0.0f;
	data[start_id].est_dist = distance(start, end);
	data[start_id].entry_pos = start;
	data[start_id].src_quad = -1;
	heap[heap_size++] = HeapData(data + start_id);

	bool end_reached = start_id == end_id;

	while(heap_size) {
		int quad_id = extractMin(heap, heap_size).ptr - data;
		if(quad_id == end_id)
			break;

		const Quad &quad1 = m_quads[quad_id];
		SearchData &data1 = data[quad_id];
		data1.is_finished = true;

		for(int n = 0; n < (int)quad1.neighbours.size(); n++) {
			int quad2_id = quad1.neighbours[n];
			const Quad &quad2 = m_quads[quad2_id];
			SearchData &data2 = data[quad2_id];

			if(data2.is_finished || quad2.is_disabled)
				continue;
			
			IRect edge = computeEdge(quad1.rect, quad2.rect);

			int2 edge_end_pos = clamp(end, edge.min, edge.max);
			MoveVector vec(data1.entry_pos, edge_end_pos);

			int2 closest_pos = clamp(data1.entry_pos + vec.vec * vec.ddiag, quad2.rect.min, quad2.rect.max - int2(1, 1));

			closest_pos = clamp(closest_pos, quad1.rect.min - int2(1, 1), quad1.rect.max);

#ifndef WALK_DIAGONAL_THROUGH_CORNERS
			// fixing problem with diagonal moves through obstacle corners
			if(quad1.rect.max.x > quad2.rect.min.x && quad1.rect.min.x < quad2.rect.max.x) {
				if(data1.entry_pos.x < closest_pos.x && closest_pos.x == quad2.rect.min.x && closest_pos.x < quad2.rect.max.x - 1)
					closest_pos.x++;
				if(data1.entry_pos.x > closest_pos.x && closest_pos.x == quad2.rect.max.x - 1 && closest_pos.x > quad2.rect.min.x)
					closest_pos.x--;
			}
			else {
				if(data1.entry_pos.y < closest_pos.y && closest_pos.y == quad2.rect.min.y && closest_pos.y < quad2.rect.max.y - 1)
					closest_pos.y++;
				if(data1.entry_pos.y > closest_pos.y && closest_pos.y == quad2.rect.max.y - 1 && closest_pos.y > quad2.rect.min.y)
					closest_pos.y--;
			}
#endif

			float dist = distance(closest_pos, data1.entry_pos) + data1.dist;
			float est_dist = distance(closest_pos, end);

			if(quad2_id == end_id) {
				end_reached = true;
				dist += est_dist;
			}

			if(do_refining? data2.dist <= dist : data2.dist + data2.est_dist <= dist + est_dist)
				continue;

			data2.dist = dist;
			data2.est_dist = est_dist;
			data2.entry_pos = closest_pos;
			data2.src_quad = quad_id;
			updateKey(heap, heap_size, &data2);
		}
	}

	if(!end_reached)
		return std::move(out);

	out.push_back(PathNode{end, end_id});
	for(int quad_id = end_id; quad_id != -1; quad_id = data[quad_id].src_quad) {
		if(out.back().point != data[quad_id].entry_pos)
			out.push_back({data[quad_id].entry_pos, quad_id});
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

			int height1 = m_quads[out[n].quad_id].max_height;
			int height2 = m_quads[out[n].quad_id].min_height;

			vector<PathNode> other = findPath(out[n].point, out[n+3].point, out[n].quad_id, out[n+3].quad_id, false);

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

	return std::move(out);
}

vector<int3> NaviMap::findPath(const int3 &start, const int3 &end) const {
	int start_id = findQuad(start);
	int end_id = findQuad(end);

	vector<PathNode> input = findPath(start.xz(), end.xz(), start_id, end_id, true);
	vector<int3> path;

	if(input.empty())
		return path;

	for(int n = 0; n < (int)input.size() - 1; n++) {
		const IRect &src_quad = m_quads[input[n + 0].quad_id].rect;
		const IRect &dst_quad = m_quads[input[n + 1].quad_id].rect;

		int2 src = input[n + 0].point;
		int2 dst = input[n + 1].point;
		MoveVector vec(src, dst);
		MoveVector prev_vec = path.empty()? MoveVector() : MoveVector(path.back().xz(), src);
		
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

#ifndef WALK_DIAGONAL_THROUGH_CORNERS
		if(is_horizontal)
				pdst.x = clamp(pdst.x, dst_quad.min.x, dst_quad.max.x - 1);
		else
				pdst.y = clamp(pdst.y, dst_quad.min.y, dst_quad.max.y - 1);
#endif

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

		int src_height = m_quads[input[n + 0].quad_id].max_height;
		int dst_height = m_quads[input[n + 1].quad_id].max_height;
		path.push_back(asXZY(src, src_height));
		path.push_back(asXZY(mid, src_height));
		path.push_back(asXZY(pdst, max(src_height, dst_height)));
	}
	path.push_back(asXZY(input.back().point, m_quads[input.back().quad_id].max_height));
	if(path.size() <= 2)
		return std::move(path);

	vector<int3> simplified;
	simplified.push_back(path[0]);
	int2 prev(0, 0);
	int prev_height = 0;
	bool height_changed = false;

	for(int n = 1; n < (int)path.size(); n++) {
		int2 vec = MoveVector(simplified.back().xz(), path[n].xz()).vec;
		if(vec == int2(0, 0))
			continue;

		if(vec == prev && path[n].y == prev_height && !height_changed) {
			simplified.back() = path[n];
			height_changed = false;
		}
		else {
			prev_height = path[n].y;
			prev = vec;
			height_changed = true;
			simplified.push_back(path[n]);
		}
	}

	return std::move(simplified);
}

void NaviMap::visualize(gfx::SceneRenderer &renderer, bool borders) const {
	for(int n = 0; n < (int)m_quads.size(); n++) {
		if(m_quads[n].is_disabled)
			continue;
		const IRect &rect = m_quads[n].rect;
		int height = m_quads[n].max_height;

		FBox bbox(asXZY(rect.min, height), asXZY(rect.max, height));
		bbox.min.x += 0.5f;
		bbox.min.z += 0.5f;

		renderer.addBox(bbox, Color(70, 220, 200, 80), true);
		if(borders)
			renderer.addBox(bbox, Color(255, 255, 255, 100));
	}
}

void NaviMap::visualizePath(const vector<int3> &path, int agent_size, gfx::SceneRenderer &renderer) const {
	if(path.empty())
		return;

	IBox box(0, 0, 0, agent_size, 0, agent_size);
	renderer.addBox(box + path.front(), Color::red);

	for(int n = 1; n < (int)path.size(); n++) {
		int3 begin = path[n - 1], end = path[n];
		bool first = true;

		IBox start_box = box + begin, end_box = box + end;
		renderer.addBox(end_box, Color::red);
		MoveVector vec(begin.xz(), end.xz());

		if(vec.vec == int2(1, 1) || vec.vec == int2(-1, -1)) {
			swap(start_box.min.x, start_box.max.x);
			swap(  end_box.min.x,   end_box.max.x);
		}

		renderer.addLine(start_box.min, end_box.min);
		renderer.addLine(start_box.max, end_box.max);
	}
}

void NaviMap::printInfo() const {
	printf("NaviMap(%d, %d):\n", m_size.x, m_size.y);

	int bytes = sizeof(Quad) * m_quads.size();
	for(int n = 0; n < (int)m_quads.size(); n++)
		bytes += m_quads[n].neighbours.size() * sizeof(int);
	printf("  quads(%d): %.0f KB\n", (int)m_quads.size(), double(bytes) / 1024.0);
	printf("  sizeof(Quad): %d\n", (int)sizeof(Quad));

	if(0) for(int n = 0; n < (int)m_quads.size(); n++) {
		const Quad &quad = m_quads[n];
		printf("%d: (%d %d %d %d): ", n, quad.rect.min.x, quad.rect.min.y, quad.rect.max.x, quad.rect.max.y);
		for(int i = 0; i < (int)quad.neighbours.size(); i++)
			printf("%d ", quad.neighbours[i]);
		printf("\n");
	}
}
