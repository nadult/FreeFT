// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "grid.h"

int Grid::findAny(const FBox &box, int ignored_id, int flags) const {
	IRect grid_box = nodeCoords(box);

	for(int y = grid_box.y(); y <= grid_box.ey(); y++)
		for(int x = grid_box.x(); x <= grid_box.ex(); x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!node.size || !flagTest(node.obj_flags, flags) || !areOverlapping(box, node.bbox))
				continue;

			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, ignored_id, flags);

			for(int n = 0; n < count; n++)
				if(areOverlapping(box, objects[n]->bbox))
					return objects[n] - &m_objects[0];

			if(node.is_dirty)
				updateNode(node_id);	
		}

	return -1;
}
	
void Grid::findAll(vector<int> &out, const FBox &box, int ignored_id, int flags) const {
	IRect grid_box = nodeCoords(box);

	for(int y = grid_box.y(); y <= grid_box.ey(); y++)
		for(int x = grid_box.x(); x <= grid_box.ex(); x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!flagTest(node.obj_flags, flags) || !areOverlapping(box, node.bbox))
				continue;
			bool anything_found = false;

			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, ignored_id, flags);

			for(int n = 0; n < count; n++)
				if(areOverlapping(box, objects[n]->bbox)) {
					if(objects[n]->node_id == -1)
						disableOverlap(objects[n]);
					out.push_back(objects[n] - &m_objects[0]);
					anything_found = true;
				}

			if(!anything_found && node.is_dirty)
				updateNode(node_id);	
		}

	clearDisables();
}

pair<int, float> Grid::trace(const Ray3F &ray, int ignored_id, int flags) const {
	float tmin = max(0.0f, isectDist(ray, m_bounding_box));
	float tmax = -isectDist(negate(ray), m_bounding_box);
	return trace(ray, tmin, tmax, ignored_id, flags);
}

pair<int, float> Grid::trace(const Segment3F &segment, int ignored_id, int flags) const {
	float tmin = max(0.0f, isectDist(segment, m_bounding_box));
	float tmax = min(segment.length(), -isectDist(negate(*segment.asRay()), m_bounding_box));
	return trace(*segment.asRay(), tmin, tmax, ignored_id, flags);
}
	
pair<int, float> Grid::trace(const Ray3F &ray, float tmin, float tmax, int ignored_id, int flags) const {
	float3 p1 = ray.at(tmin), p2 = ray.at(tmax);
	int2 pos = worldToGrid((int2)p1.xz()), end = worldToGrid((int2)p2.xz());
	
	//TODO: verify for rays going out of grid space
	if(!isInsideGrid(pos) || !isInsideGrid(end))
		return make_pair(-1, fconstant::inf);

	// Algorithm idea from: RTCD by Christer Ericson
	int dx = end.x > pos.x? 1 : end.x < pos.x? -1 : 0;
	int dz = end.y > pos.y? 1 : end.y < pos.y? -1 : 0;

	float cell_size = (float)node_size;
	float inv_cell_size = 1.0f / cell_size;
	float lenx = fabs(p2.x - p1.x);
	float lenz = fabs(p2.z - p1.z);

	float minx = float(node_size) * floorf(p1.x * inv_cell_size), maxx = minx + cell_size;
	float minz = float(node_size) * floorf(p1.z * inv_cell_size), maxz = minz + cell_size;
	float tx = (p1.x > p2.x? p1.x - minx : maxx - p1.x) / lenx;
	float tz = (p1.z > p2.z? p1.z - minz : maxz - p1.z) / lenz;

	float deltax = cell_size / lenx;
	float deltaz = cell_size / lenz;

	int out = -1;
	float out_dist = tmax + fconstant::epsilon;

	while(true) {
		int node_id = nodeAt(pos);
		const Node &node = m_nodes[node_id];

		if(flagTest(node.obj_flags, flags) && isectDist(ray, node.bbox) < out_dist) {
			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, ignored_id, flags);

			for(int n = 0; n < count; n++) {
				float dist = isectDist(ray, objects[n]->bbox);
				if(dist < out_dist) {
					out_dist = dist;
					out = objects[n] - &m_objects[0];
				}
			}	
			
			if(node.is_dirty)
				updateNode(node_id);
		}

		if(tx <= tz || dz == 0) {
			if(pos.x == end.x)
				break;
			tx += deltax;
			pos.x += dx;
		}
		else {
			if(pos.y == end.y)
				break;
			tz += deltaz;
			pos.y += dz;
		}
		float ray_pos = tmin + max((tx - deltax) * lenx, (tz - deltaz) * lenz);
		if(ray_pos >= out_dist)
			break;
	}

	return make_pair(out, out_dist);
}

void Grid::traceCoherent(const vector<Segment3F> &segments, vector<pair<int, float>> &out, int ignored_id, int flags) const {
	out.resize(segments.size(), make_pair(-1, fconstant::inf));

	if(segments.empty())
		return;

	int2 start, end; {
		float3 pmin(fconstant::inf, fconstant::inf, fconstant::inf);
		float3 pmax(-fconstant::inf, -fconstant::inf, -fconstant::inf);

		for(int s = 0; s < (int)segments.size(); s++) {
			const Segment3F &segment = segments[s];
			float tmin = max(0.0f, isectDist(segment, m_bounding_box));
			float tmax = min(segment.length(), -isectDist(negate(*segment.asRay()), m_bounding_box));

			float3 p1 = segment.at(tmin), p2 = segment.at(tmax);
			pmin = vmin(pmin, vmin(p1, p2));
			pmax = vmax(pmax, vmax(p1, p2));
		}

		start = worldToGrid((int2)pmin.xz());
		end = worldToGrid((int2)pmax.xz());
		start = vmax(start, int2(0, 0));
		end = vmin(end, int2(m_size.x - 1, m_size.y - 1));
	}

	float max_dist = -fconstant::inf;
	IntervalF idir[3], origin[3]; {
		auto first = *segments.front().asRay();
		auto first_idir = first.invDir();

		idir  [0] = first_idir.x; idir  [1] = first_idir.y; idir  [2] = first_idir.z;
		origin[0] = first.origin().x; origin[1] = first.origin().y; origin[2] = first.origin().z;

		for(int s = 1; s < (int)segments.size(); s++) {
			const Segment3F &segment = segments[s];
			const auto ray = *segment.asRay();
			const auto ray_idir = ray.invDir();

			float tidir[3] = { ray_idir.x, ray_idir.y, ray_idir.z };
			float torigin[3] = { segment.from.x, segment.from.y, segment.from.z };

			max_dist = max(max_dist, segment.length());
			for(int i = 0; i < 3; i++) {
				idir  [i] = IntervalF(min(idir  [i].min, tidir  [i]), max(idir  [i].max, tidir  [i]));
				origin[i] = IntervalF(min(origin[i].min, torigin[i]), max(origin[i].max, torigin[i]));
			}
		}
	}

	for(int x = start.x; x <= end.x; x++) for(int z = start.y; z <= end.y; z++) {
		int node_id = nodeAt(int2(x, z));
		const Node &node = m_nodes[node_id];

		if(flagTest(node.obj_flags, flags) && intersection(idir, origin, node.bbox) < max_dist) {
			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, ignored_id, flags);

			for(int n = 0; n < count; n++) {
				if(intersection(idir, origin, objects[n]->bbox) < max_dist) {
					for(int s = 0; s < (int)segments.size(); s++) {
						const Segment3F &segment = segments[s];
						float dist = isectDist(segment, objects[n]->bbox);
						if(dist < out[s].second) {
							out[s].second = dist;
							out[s].first = objects[n] - &m_objects[0];
						}
					}
				}
			}
			
			if(node.is_dirty)
				updateNode(node_id);
		}
	}
}

void Grid::findAll(vector<int> &out, const IRect &view_rect, int flags) const {
	IRect grid_box(0, 0, m_size.x, m_size.y);

	for(int y = grid_box.y(); y < grid_box.ey(); y++) {
		const int2 &row_rect = m_row_rects[y];
		if(row_rect.x >= view_rect.ey() || row_rect.y <= view_rect.y())
			continue;
		
		for(int x = grid_box.x(); x < grid_box.ex(); x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];
	
			if(!flagTest(node.obj_flags, flags) || !areOverlapping(view_rect, node.rect))
				continue;

			bool anything_found = false;
			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, -1, flags);

			for(int n = 0; n < count; n++) {
				if(areOverlapping(view_rect, objects[n]->rect())) {
					if(objects[n]->node_id == -1)
						disableOverlap(objects[n]);
					out.push_back(objects[n] - &m_objects[0]);
					anything_found = true;
				}
			}

			if(!anything_found && node.is_dirty)
				updateNode(node_id);	
		}
	}

	clearDisables();
}

int Grid::pixelIntersect(const int2 &screen_pos, bool (*pixelTest)(const ObjectDef&, const int2&), int flags) const {
	IRect grid_box(0, 0, m_size.x, m_size.y);
	
	int best = -1;
	FBox best_box;

	for(int y = grid_box.y(); y < grid_box.ey(); y++) {
		const int2 &row_rect = m_row_rects[y];
		if(row_rect.x >= screen_pos.y || row_rect.y <= screen_pos.y)
			continue;

		for(int x = grid_box.x(); x < grid_box.ex(); x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!flagTest(node.obj_flags, flags) || !node.rect.containsPixel(screen_pos))
				continue;
			if(node.is_dirty)
				updateNode(node_id);	

			const Object *objects[node.size];
			int count = extractObjects(node_id, objects, -1, flags);

			for(int n = 0; n < count; n++)
				if(objects[n]->rect().containsPixel(screen_pos) && pixelTest(*objects[n], screen_pos)) {
					if(best == -1 || drawingOrder(objects[n]->bbox, best_box) == 1) {
						best = objects[n] - &m_objects[0];
						best_box = objects[n]->bbox;
					}
				}
		}
	}

	return best;
}

