/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "navi_map.h"
#include "navi_heightmap.h"
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

void NaviMap::extractQuads(const PodArray<u8> &bitmap, const int2 &bsize, int sx, int sy) {
	int2 size(min((int)sector_size, bsize.x - sx), min((int)sector_size, bsize.y - sy));

	int pixels = 0;
	PodArray<short> counts(sector_size * size.y * 2);
	short *skips = counts.data() + sector_size * size.y;

	for(int y = 0; y < size.y; y++) {
		int yoff = y * sector_size;
		for(int x = 0; x < size.x; x++) {
			if(bitmap[sx + x + (sy + y) * bsize.x]) {
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
		u8 min_height = bitmap[rect.min.x + rect.min.y * bsize.x];
		u8 max_height = min_height;

		for(int y = rect.min.y; y < rect.max.y; y++)
			for(int x = rect.min.x; x < rect.max.x; x++) {
				u8 height = bitmap[x + y * bsize.x];
				min_height = min(min_height, height);
				max_height = max(max_height, height);
			}

		m_quads.push_back(Quad(rect, min_height, max_height));
		listInsert<Quad, &Quad::node>(m_quads, m_sectors[findSector(rect.min)], (int)m_quads.size() - 1);

		pixels -= best.width() * best.height();
	}
}

static const IRect computeEdge(const IRect &quad1, const IRect &quad2) {
	bool is_horizontal = quad1.max.x > quad2.min.x && quad1.min.x < quad2.max.x;
	IRect edge(vmax(quad1.min, quad2.min), vmin(quad1.max, quad2.max));

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

	if(quad1.min_height <= quad2.max_height + 1 && quad2.min_height <= quad1.max_height + 1 && areAdjacent(quad1.rect, quad2.rect)) {
		quad1.neighbours.push_back(quad2_id);
		quad2.neighbours.push_back(quad1_id);
	}
}

void NaviMap::update(const NaviHeightmap &heightmap) {
	int2 bsize = heightmap.dimensions();
	m_size = int2(bsize.x + sector_size - 1, bsize.y + sector_size - 1) / sector_size;
	m_quads.clear();

	m_sectors.resize(m_size.x * m_size.y);

	printf("Creating navigation map: "); fflush(stdout);
	double time = getTime();

	int level_count = heightmap.levelCount();

	enum { max_levels = 256 };

	vector<PodArray<u8>> bitmaps(level_count);
	vector<int> level_pixels(max_levels, 0);

	for(int l = 0; l < level_count; l++) {
		bitmaps[l].resize(bsize.x * bsize.y);
		PodArray<u8> &bitmap = bitmaps[l];
		memset(bitmap.data(), 0, bitmap.dataSize());

		for(int y = 0; y < bsize.y; y++)
			for(int x = 0; x < bsize.x; x++)
				if(heightmap.test(x, y, l, m_agent_size)) {
					u8 height = heightmap(x, y, l);
					bitmap[x + y * bsize.x] = height;
					level_pixels[height]++;
				}
	}

	for(int h = 1; h < max_levels; h++) {
		int pixel_count = level_pixels[h];
		if(!pixel_count)
			continue;

		PodArray<u8> bitmap(bsize.x * bsize.y);
		memset(bitmap.data(), 0, bitmap.dataSize());

		for(int l = 0; l < level_count; l++) {
			const PodArray<u8> &lbitmap = bitmaps[l];
			for(int i = 0; i < bitmap.size(); i++)
				if(lbitmap[i] == h)
					bitmap[i] = h;
		}

		PodArray<u8> subbitmap = bitmap;
		memset(subbitmap.data(), 0, subbitmap.dataSize());

		vector<int2> positions;
		int max_diff = 0;
		int start_line = 0;

		while(pixel_count) {
			IRect subrect;
			positions.clear();
			int hmin, hmax;

			for(int y = start_line; y < bsize.y; y++) {
				for(int x = 0; x < bsize.x; x++) 
					if(bitmap[x + y * bsize.x]) {
						hmin = hmax = bitmap[x + y * bsize.x];
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

				int offset = pos.x + pos.y * bsize.x;
				int height = bitmap[offset];
				if(!height)
					continue;
				if(max(height - hmin, hmax - height) > max_diff)
					continue;

				hmin = min(hmin, height);
				hmax = max(hmax, height);

				subrect.min = vmin(subrect.min, pos);
				subrect.max = vmax(subrect.max, pos);
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
					pos.y > 0? bitmap[offset - bsize.x] : 0,
					pos.x < bsize.x - 1? bitmap[offset + 1] : 0,
					pos.y < bsize.y - 1? bitmap[offset + bsize.x] : 0
				};

				for(int n = 0; n < 4; n++) {
					int height = neighbours[n];
					if(height && max(height - hmin, hmax - height) <= max_diff)
						positions.push_back(pos + offsets[n]);
				}
			}
			subrect.max += int2(1, 1);

			for(int sy = 0; sy < bsize.y; sy += sector_size)
				for(int sx = 0; sx < bsize.x; sx += sector_size)
					if(areOverlapping(subrect, IRect(sx, sy, sx + sector_size, sy + sector_size)))
						extractQuads(subbitmap, bsize, sx, sy);

			for(int y = subrect.min.y; y < subrect.max.y; y++)
				memset(subbitmap.data() + y * bsize.x + subrect.min.x, 0, subrect.width());
		}
	}
	m_static_count = (int)m_quads.size();

	vector<int> indices;
	for(int i = 0; i < (int)m_quads.size(); i++) {
		indices.clear();
		const Quad &quad = m_quads[i];
		IBox bbox = quad.box();

		findQuads(IBox(bbox.min - int3(1, 1, 1), bbox.max + int3(1, 1, 1)), indices);
		for(int j = 0; j < (int)indices.size(); j++)
			if(indices[j] < i)
				addAdjacencyInfo(i, indices[j]);
	}

	for(int n = 0; n < (int)m_quads.size(); n++)
		m_quads[n].static_ncount = (int)m_quads[n].neighbours.size();
	printf("%d quads (%.2f seconds)\n", (int)m_quads.size(), getTime() - time);
}

void NaviMap::addCollider(int parent_id, const IRect &rect, int collider_id) {
	Quad *parent = &m_quads[parent_id];
	IRect prect = parent->rect;
	IRect crect(vmax(rect.min, prect.min), vmin(rect.max, prect.max));
	if(crect.empty())
		return;

	IRect rects[4];
	rects[0] = IRect(prect.min, int2(crect.max.x, crect.min.y));
	rects[1] = IRect(int2(crect.max.x, prect.min.y), int2(prect.max.x, crect.max.y));
	rects[2] = IRect(int2(prect.min.x, crect.min.y), int2(crect.min.x, prect.max.y));
	rects[3] = IRect(int2(crect.min.x, crect.max.y), prect.max);

	parent->is_disabled = true;
	parent->collider_id = collider_id;

	int first_id = (int)m_quads.size();
	u8 min_height = parent->min_height;
	u8 max_height = parent->max_height;

	for(int n = 0; n < arraySize(rects); n++)
		if(!rects[n].empty()) {
			m_quads.push_back(Quad(rects[n], min_height, max_height));
			listInsert<Quad, &Quad::node>(m_quads, m_sectors[findSector(rects[n].min)], (int)m_quads.size() - 1);
		}
	parent = &m_quads[parent_id];

	int count = 0;
	for(int n = first_id; n < (int)m_quads.size(); n++) {
		for(int i = 0; i < (int)parent->neighbours.size(); i++)
			addAdjacencyInfo(n, parent->neighbours[i]);
		for(int i = first_id; i < n; i++)
			addAdjacencyInfo(n, i);
	}
}

void NaviMap::addCollider(const IBox &box, int collider_id) {
	if(box.empty())
		return;

	IBox ext_box(box.min - int3(m_agent_size - 1, 2, m_agent_size - 1), box.max);
	IRect ext_rect(ext_box.min.xz(), ext_box.max.xz());
	vector<int> indices;
	
	findQuads(ext_box, indices);
	for(int n = 0; n < (int)indices.size(); n++) {
		const Quad &quad = m_quads[indices[n]];
		if(!quad.is_disabled && quad.min_height <= ext_box.max.y && quad.max_height >= ext_box.min.y)
			addCollider(indices[n], ext_rect, collider_id);
	}
}

void NaviMap::removeColliders() {
	for(int n = m_static_count; n < (int)m_quads.size(); n++) {
		Quad &quad = m_quads[n];
		listRemove<Quad, &Quad::node>(m_quads, m_sectors[findSector(quad.rect.min)], n);
	}
	m_quads.resize(m_static_count);

	for(int n = 0; n < (int)m_quads.size(); n++) {
		Quad &quad = m_quads[n];
		quad.neighbours.resize(quad.static_ncount);
		quad.is_disabled = false;
		quad.collider_id = -1;
	}
}
	
void NaviMap::updateReachability() {
	//TODO: this is probably too slow
	//FWK_PROFILE("NaviMap::updateReachability");

	vector<int> groups(m_quads.size(), -1);

	int new_group_id = 0;
	vector<int> stack;
	stack.reserve(m_quads.size());

	for(int n = 0; n < (int)m_quads.size(); n++) {
		Quad &quad = m_quads[n];

		if(groups[n] != -1 || quad.is_disabled)
			continue;

		int group_id = new_group_id++;

		stack.clear();
		stack.push_back(n);

		while(!stack.empty()) {
			int quad_id = stack.back();
			Quad &quad = m_quads[quad_id];
			stack.pop_back();

			if(quad.is_disabled || groups[quad_id] != -1)
				continue;

			groups[quad_id] = group_id;
			for(int i = 0; i < (int)quad.neighbours.size(); i++) {
				int nid = quad.neighbours[i];
				Quad &nquad = m_quads[nid];
				if(!nquad.is_disabled && groups[nid] == -1)
					stack.push_back(nid);
			}
		}
	}

	for(int n = 0; n < (int)m_quads.size(); n++)
		m_quads[n].group_id = groups[n];
}

bool NaviMap::isReachable(int src_id, int target_id) const {
	int quad_id[2] = { src_id, target_id };

	if(quad_id[0] == -1 || quad_id[1] == -1)
		return false;

	vector<int> groups[2];

	for(int p = 0; p < 2; p++) {
		const Quad &quad = m_quads[quad_id[p]];
		if(quad.is_disabled) {
			vector<int> to_visit;
			to_visit.push_back(quad_id[p]);
			int collider_id = quad.collider_id;

			// Visiting all disabled quads with same collider
			for(int i = 0; i < (int)to_visit.size(); i++) {
				const Quad tquad = m_quads[to_visit[i]];

				for(int n = 0; n < (int)tquad.neighbours.size(); n++) {
					const Quad &neighbour = m_quads[tquad.neighbours[n]];
					if(neighbour.is_disabled) {
						bool found = false;
						for(int j = 0; j < (int)to_visit.size(); j++)
							if(to_visit[j] == tquad.neighbours[n]) {
								found = true;
								break;
							}
						if(!found)
							to_visit.push_back(tquad.neighbours[n]);
					}
					else
						groups[p].push_back(neighbour.group_id);
				}
			}
		}
		else
			groups[p].push_back(quad.group_id);
	}

	for(int i = 0; i < (int)groups[0].size(); i++)
		for(int j = 0; j < (int)groups[1].size(); j++)
			if(groups[0][i] == groups[1][j])
				return true;
	
	return false;
}

bool NaviMap::isReachable(const int3 &source, const int3 &target) const {
	return isReachable(findQuad(source, -1, true), findQuad(target, -1, true));
}

bool NaviMap::findClosestPos(int3 &out, const int3 &pos, int source_height, const IBox &target_box) const {
	IBox enlarged_box = target_box;
	enlarged_box.min -= int3(1, source_height - 1, 1);
	enlarged_box.max += int3(1, 0, 1);

	vector<int> quads;
	findQuads(enlarged_box, quads);
	
	int3 closest_pos = pos;
	float min_distance = fconstant::inf, min_distance2 = 0.0f;
	FRect ftarget_rect((float2)target_box.min.xz(), (float2)target_box.max.xz());

	int3 clip_pos = pos;
	if(pos.x + m_agent_size < target_box.min.x)
		clip_pos.x = target_box.min.x - m_agent_size;
	else if(pos.x > target_box.max.x)
		clip_pos.x = target_box.max.x;
	if(pos.z + m_agent_size < target_box.min.z)
		clip_pos.z = target_box.min.z - m_agent_size;
	else if(pos.z > target_box.max.z)
		clip_pos.z = target_box.max.z;

	int src_quad = findQuad(pos, -1, true);

	for(int n = 0; n < (int)quads.size(); n++) {
		const Quad &quad = m_quads[quads[n]];
		if(quad.min_height > enlarged_box.max.y || quad.max_height < enlarged_box.min.y || quad.is_disabled)
			continue;
		
		int3 new_pos = vclamp(clip_pos,	asXZY(quad.rect.min, quad.min_height),
								 		asXZY(quad.rect.max - int2(1, 1), quad.max_height));

		float dist  = distanceSq(ftarget_rect, FRect((float2)new_pos.xz(), float2(new_pos.xz()) + float2(m_agent_size, m_agent_size)));
		float dist2 = distanceSq((float3)new_pos, (float3)pos);

		if(dist < min_distance || (fabs(dist - min_distance) <= fconstant::epsilon && dist2 < min_distance2)) {
			if(isReachable(src_quad, quads[n])) {
				closest_pos = new_pos;
				min_distance = dist;
				min_distance2 = dist2;
			}
		}
	}

	out = closest_pos;
	return min_distance < fconstant::inf;
}

void NaviMap::findQuads(const IBox &box, vector<int> &out, bool cheap_filter) const {
	int2 min_sector = vmax(box.min.xz() / sector_size, int2(0, 0));
	int2 max_sector = vmin(box.max.xz() / sector_size, m_size);

	for(int x = min_sector.x; x <= max_sector.x; x++)
		for(int y = min_sector.y; y <= max_sector.y; y++) {
			int node = m_sectors[x + y * m_size.x].head;
			while(node != -1) {
				const Quad &quad = m_quads[node];
				if(cheap_filter || areOverlapping(IBox(asXZY(quad.rect.min, (int)quad.min_height), asXZY(quad.rect.max, (int)quad.max_height)), box))
					out.push_back(node);
				node = quad.node.next;
			}
		}
}

int NaviMap::findQuad(const int3 &pos, int filter_collider, bool find_disabled) const {
	int best_quad = -1;
	int best_height = -1;

	int2 sector_pos = pos.xz() / sector_size;
	if(sector_pos.x < 0 || sector_pos.y < 0 || sector_pos.x >= m_size.x || sector_pos.y >= m_size.y)
		return -1;

	int node = m_sectors[sector_pos.x + sector_pos.y * m_size.x].tail;

	while(node != -1) {
		const Quad &quad = m_quads[node];
		if(quad.rect.isInside(pos.xz()) && (!quad.is_disabled || quad.collider_id == filter_collider || find_disabled)
				&& pos.y >= quad.min_height) {

			if(pos.y <= quad.max_height)
				return node;

			if(quad.max_height > best_height) {
				best_height = quad.max_height;
				best_quad = node;
			}
		}
		node = quad.node.prev;
	}

	return best_quad;
}
	
// powoduje problemy jak się przechodzi diagonalnie przez rogi encji
// (gracz się zatrzymuje bo jest kolizja)
// This should be solved by creating paths that are trying to be in the middle
// between colliders
//#define WALK_DIAGONAL_THROUGH_CORNERS


static float distance(const int2 &a, const int2 &b) {
	int dist_x = fwk::abs(a.x - b.x), dist_y = fwk::abs(a.y - b.y);
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
		HeapData() = default;

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

	struct SearchInfo {
		SearchInfo(int count) :m_data(count), m_init_map((count + 31) / 32) {
			memset(m_init_map.data(), 0, m_init_map.dataSize());
		}

		SearchData &operator[](int idx) {
			int map_idx = idx / 32;
			int bit = 1 << (idx & 31);

			SearchData &out = m_data[idx];

			if(!(m_init_map[map_idx] & bit)) {
				m_init_map[map_idx] |= bit;
				out.dist = fconstant::inf;
				out.heap_pos = -1;
				out.is_finished = false;
			}

			return out;
		}

		PodArray<SearchData> m_data;
		PodArray<int> m_init_map;
	};

#undef UPDATE
}

vector<NaviMap::PathNode> NaviMap::findPath(const int2 &start, const int2 &end, int start_id, int end_id,
											bool do_refining, int filter_collider) const {
	vector<PathNode> out;
	
	if(filter_collider == -1)
		filter_collider = -2;

	if(start_id == -1 || end_id == -1) //TODO: info that path not found
		return out;

	int is_initialized[((int)m_quads.size() + 31) / 32];
	memset(is_initialized, 0, ((int)m_quads.size() + 31) / 32);

	SearchInfo data((int)m_quads.size());

	HeapData heap[(int)m_quads.size()];
	int heap_size = 0;

	data[start_id].dist = 0.0f;
	data[start_id].est_dist = distance(start, end);
	data[start_id].entry_pos = start;
	data[start_id].src_quad = -1;
	heap[heap_size++] = HeapData(&data[start_id]);

	bool end_reached = start_id == end_id;

	while(heap_size) {
		int quad_id = extractMin(heap, heap_size).ptr - &data[0];
		if(quad_id == end_id)
			break;

		const Quad &quad1 = m_quads[quad_id];
		SearchData &data1 = data[quad_id];
		data1.is_finished = true;

		for(int n = 0; n < (int)quad1.neighbours.size(); n++) {
			int quad2_id = quad1.neighbours[n];
			const Quad &quad2 = m_quads[quad2_id];
			SearchData &data2 = data[quad2_id];

			if(data2.is_finished || (quad2.is_disabled && quad2.collider_id != filter_collider))
				continue;
			
			IRect edge = computeEdge(quad1.rect, quad2.rect);

			int2 edge_end_pos = vclamp(end, edge.min, edge.max);
			MoveVector vec(data1.entry_pos, edge_end_pos);

			int2 closest_pos = vclamp(data1.entry_pos + vec.vec * vec.ddiag, quad2.rect.min, quad2.rect.max - int2(1, 1));

			closest_pos = vclamp(closest_pos, quad1.rect.min - int2(1, 1), quad1.rect.max);

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
		return out;

	out.push_back(PathNode{end, end_id});
	for(int quad_id = end_id; quad_id != -1; quad_id = data[quad_id].src_quad) {
		if(out.back().point != data[quad_id].entry_pos)
			out.push_back({data[quad_id].entry_pos, quad_id});
	}
	std::reverse(out.begin(), out.end());
	
	//TODO: it's very costly
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

			vector<PathNode> other = findPath(out[n].point, out[n+3].point, out[n].quad_id, out[n+3].quad_id, false, filter_collider);

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

bool NaviMap::findPath(vector<int3> &out, const int3 &start, const int3 &end, int filter_collider) const {
	FWK_PROFILE_RARE("NaviMap::findPath");

	int start_id = findQuad(start, filter_collider);
	int end_id = findQuad(end, filter_collider);

	if(!isReachable(start_id, end_id))
		return false;

	vector<PathNode> input = findPath(start.xz(), end.xz(), start_id, end_id, true, filter_collider);
	
	vector<int3> path;
	path.reserve(input.size() * 3);

	if(input.empty())
		return false;

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
		pdst = vclamp(pdst, src_quad.min, src_quad.max - int2(1,1));

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

	vector<int3> simplified;
	simplified.push_back(path[0]);
	int2 last_vec(0, 0);

	for(int n = 1; n < (int)path.size(); n++) {
		const int3 &cur = path[n];
		int3 &prev = simplified.back();

		int2 cur_vec = MoveVector(prev.xz(), cur.xz()).vec;
		if(cur_vec == int2(0, 0))
			continue;

		if(cur_vec != last_vec || cur.y != prev.y) {
			simplified.push_back(cur);
			last_vec = cur_vec;
		}
		else
			prev = cur;
	}

	out = std::move(simplified);
	return true;
}

void NaviMap::visualize(SceneRenderer &renderer, bool borders) const {
	for(int n = 0; n < (int)m_quads.size(); n++) {
		if(m_quads[n].is_disabled)
			continue;
		const IRect &rect = m_quads[n].rect;
		int height = m_quads[n].max_height;

		FBox bbox((float3)asXZY(rect.min, height), (float3)asXZY(rect.max, height));
		bbox.min.x += 0.3f;
		bbox.min.z += 0.3f;

		renderer.addBox(bbox, Color(70, 220, 200, 80), true);
		if(borders)
			renderer.addBox(bbox, Color(255, 255, 255, 100));
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
