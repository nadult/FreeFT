/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "occluder_map.h"
#include "sys/xml.h"
#include <map>
#include <algorithm>

OccluderMap::OccluderMap(Grid &grid) :m_grid(grid) { }

int OccluderMap::addOccluder(int representative_id) {
	DASSERT(representative_id >= 0 && representative_id < m_grid.size());
	auto &representative = m_grid[representative_id];
	DASSERT(representative.occluder_id == -1);

	int occluder_id = (int)m_occluders.size();
	DASSERT(occluder_id < max_occluders);
	m_occluders.push_back(Occluder());
	Occluder &occluder = m_occluders.back();

	vector<int> objects, temp;
	objects.reserve(1024);
	temp.reserve(128);

	representative.occluder_id = occluder_id;
	objects.push_back(representative_id);
	occluder.bbox = representative.bbox;
	int min_height = representative.bbox.min.y;

	while(!objects.empty()) {
		int object_id = objects.back();
		objects.pop_back();
		occluder.objects.push_back(object_id);

		FBox box = m_grid[object_id].bbox;
		occluder.bbox = sum(occluder.bbox, box);

		box.min -= float3(1, 0, 1);
		box.max += float3(1, 256, 1);
		box.min.y = max(box.min.y - 1, (float)min_height);
	
		temp.clear();
		m_grid.findAll(temp, box, object_id);

		for(int n = 0; n < (int)temp.size(); n++) {
			auto &object = m_grid[temp[n]];
			if(object.occluder_id != -1) {
				Occluder &nextocc = m_occluders[object.occluder_id];
				continue;
			}
			object.occluder_id = occluder_id;
			objects.push_back(temp[n]);
		}
	}

	return occluder_id;
}

void OccluderMap::removeOccluder(int occluder_id) {
	DASSERT(occluder_id >= 0 && occluder_id < (int)m_occluders.size());
#define FIX_INDEX(ref) { if(ref == occluder_id) ref = -1; else if(ref > occluder_id) ref--; }

	for(int n = 0; n < m_grid.size(); n++) {
		auto &object = m_grid[n];
		if(!object.ptr)
			continue;

		FIX_INDEX(object.occluder_id);
		FIX_INDEX(object.occluded_by);
	}

#undef FIX_INDEX

	m_occluders.erase(m_occluders.begin() + occluder_id);
}

void OccluderMap::clear() {
	while(size() > 0)
		removeOccluder(size() - 1);
}

static bool bboxOrderYXZ(const FBox &a, const FBox &b) {
	return a.min.y == b.min.y? a.min.x == b.min.x?
		a.min.z < b.min.z : a.min.x < b.min.x : a.min.y < b.min.y;
}

static bool bboxOrderYZX(const FBox &a, const FBox &b) {
	return a.min.y == b.min.y? a.min.z == b.min.z?
		a.min.x < b.min.x : a.min.z < b.min.z : a.min.y < b.min.y;
}

vector<FBox> OccluderMap::computeBBoxes(int occluder_id, bool minimize) const {
	DASSERT(occluder_id >= 0 && occluder_id < size());

	const Occluder &occluder = m_occluders[occluder_id];
	PodArray<FBox> bboxes(occluder.objects.size());
	PodArray<FBox> temp(occluder.objects.size());
	int count = (int)occluder.objects.size(), tcount = 0;

	for(int n = 0; n < count; n++)
		bboxes[n] = m_grid[occluder.objects[n]].bbox;

	std::sort(bboxes.data(), bboxes.end(), bboxOrderYXZ);
	FBox current = bboxes[0];
	for(int n = 1; n < count; n++) {
		FBox next = bboxes[n];
		if(current.min.y == next.min.y && current.max.y == next.max.y &&
			current.min.x == next.min.x && current.max.x == next.max.x && current.max.z == next.min.z)
			current.max.z = next.max.z;
		else {
			temp[tcount++] = current;
			current = next;
		}
	}
	temp[tcount++] = current;
	bboxes.swap(temp);
	count = tcount;
	std::sort(bboxes.data(), bboxes.data() + count, bboxOrderYZX);

	tcount = 0;
	current = bboxes[0];
	for(int n = 1; n < count; n++) {
		FBox next = bboxes[n];
		if(current.min.y == next.min.y && current.max.y == next.max.y &&
			current.min.z == next.min.z && current.max.z == next.max.z && current.max.x == next.min.x)
			current.max.x = next.max.x;
		else {
			temp[tcount++] = current;
			current = next;
		}
	}
	temp[tcount++] = current;
	bboxes.swap(temp);
	count = tcount;
	int normal_count = count;

	// if minimize is true, then bboxes might overlap objects belonging to
	// occluders with index greater than occluder_id
	if(minimize) {
		vector<int> inds;
		inds.reserve(1024);

		for(int iters = 0; iters < 16; iters++) {
			tcount = 0;
			current = bboxes[0];

			for(int n = 0; n < count - 1; n += 2) {
				FBox merged = sum(bboxes[n], bboxes[n + 1]);

				inds.clear();
				m_grid.findAll(inds, merged);
				bool can_merge = true;

				for(int i = 0; i < (int)inds.size(); i++)
					if(m_grid[inds[i]].occluder_id < occluder_id) {
						can_merge = false;
						break;
					}

				if(can_merge)
					temp[tcount++] = merged;
				else {
					temp[tcount++] = bboxes[n];
					temp[tcount++] = bboxes[n + 1];
				}
			}
			if(count & 1)
				temp[tcount++] = bboxes[count - 1];
			bboxes.swap(temp);
			if(tcount == count)
				break;
			count = tcount;
		}
	}

	//printf("%5d -> %5d [%5d]\n", (int)occluder.objects.size(), count, normal_count);
	return vector<FBox>(bboxes.data(), bboxes.data() + count);
}

bool OccluderMap::verifyBBoxes(int occluder_id, const vector<FBox> &bboxes) const {
	DASSERT(occluder_id >= 0 && occluder_id < size());

	vector<int> objects = m_occluders[occluder_id].objects;
	std::sort(objects.begin(), objects.end());

	vector<int> bobjects;
	for(int n = 0; n < (int)bboxes.size(); n++)
		m_grid.findAll(bobjects, bboxes[n]);
	for(int n = 0; n < (int)bobjects.size(); n++)
		if(m_grid[bobjects[n]].occluder_id > occluder_id) {
			bobjects[n--] = bobjects.back();
			bobjects.pop_back();
		}
	std::sort(bobjects.begin(), bobjects.end());
	bobjects.resize(std::unique(bobjects.begin(), bobjects.end()) - bobjects.begin());

	return bobjects == objects;
}

void OccluderMap::loadFromXML(const XMLDocument &doc) {
	clear();

	for(int n = 0; n < m_grid.size(); n++)
		if(m_grid[n].ptr)
			ASSERT(m_grid[n].occluder_id == -1);

	XMLNode main_node = doc.child("occluders");

	if(main_node) {
		XMLNode occluder_node = main_node.child("occluder");
		vector<int> temp;
		temp.reserve(m_grid.size());

		while(occluder_node) {
			int occluder_id = size();
			m_occluders.push_back(Occluder());
			Occluder &occluder = m_occluders.back();
			
			int object_count = occluder_node.intAttrib("object_count");
			ASSERT(object_count < m_grid.size() && object_count > 0);
			occluder.objects.resize(object_count, -1);

			XMLNode box_node = occluder_node.child("box");
			bool first = true;
			while(box_node) {
				FBox bbox(box_node.float3Attrib("min"), box_node.float3Attrib("max"));
				occluder.bbox = first? bbox : sum(occluder.bbox, bbox);
				first = false;

				temp.clear();
				m_grid.findAll(temp, bbox);
				for(int n = 0; n < (int)temp.size(); n++)
					m_grid[temp[n]].occluder_id = occluder_id;
				box_node = box_node.sibling("box");
			}
			
			occluder_node = occluder_node.sibling("occluder");
		}

		vector<int> counts(m_occluders.size(), 0);
		for(int n = 0; n < m_grid.size(); n++) {
			int occ_id = m_grid[n].occluder_id;
			if(occ_id != -1 && m_grid[n].ptr) {
				Occluder &occluder = m_occluders[occ_id];
				int count = counts[occ_id];
				ASSERT(count < (int)occluder.objects.size());
				occluder.objects[count++] = n;
				counts[occ_id] = count;
			}
		}
		for(int n = 0; n < (int)counts.size(); n++)
			ASSERT(counts[n] == (int)m_occluders[n].objects.size());
	}
}

void OccluderMap::saveToXML(const PodArray<int> &tile_ids, XMLDocument &doc) const {
	XMLNode main_node = doc.addChild("occluders");
	for(int n = 0; n < (int)m_occluders.size(); n++) {
		const Occluder &occluder = m_occluders[n];
		const auto &bboxes = computeBBoxes(n, true);
		DASSERT(verifyBBoxes(n, bboxes));

		XMLNode occluder_node = main_node.addChild("occluder");
		occluder_node.addAttrib("object_count", (int)occluder.objects.size());

		for(int b = 0; b < (int)bboxes.size(); b++) {
			XMLNode box_node = occluder_node.addChild("box");
			box_node.addAttrib("min", bboxes[b].min);
			box_node.addAttrib("max", bboxes[b].max);
		}
	}
}
bool OccluderMap::isUnder(int lower_id, int upper_id) const {
	DASSERT(upper_id >= 0 && upper_id < size());
	DASSERT(lower_id >= 0 && lower_id < size());

	const Occluder &lower = m_occluders[lower_id];
	const Occluder &upper = m_occluders[upper_id];

	if(lower.bbox.min.y >= upper.bbox.max.y)
		return false;
	if(!areOverlapping(	FRect(lower.bbox.min.xz(), lower.bbox.max.xz()),
						FRect(upper.bbox.min.xz(), upper.bbox.max.xz())))
		return false;
	if(lower_id == upper_id)
		return false;

	vector<int> temp;
	temp.reserve(1024);
	
	FBox bbox = upper.bbox;
	bbox.max.y = bbox.min.y;
	bbox.min.y = 0;
	m_grid.findAll(temp, bbox);

	for(int n = 0; n < (int)temp.size(); n++)
		if(m_grid[temp[n]].occluder_id == lower_id)
			return true;
	return false;
}
	
OccluderConfig::OccluderConfig(const OccluderMap &map)
	:m_map(map) {
	update();
}
	
bool OccluderConfig::update() {
	if((int)m_states.size() != m_map.size()) {
		m_states.resize(m_map.size());
		return true;
	}

	return false;
}

bool OccluderConfig::update(const FBox &bbox) {
	//TODO: hiding when close to a door/window
	FBox test_box(bbox.min.x, bbox.min.y + 1.0f, bbox.min.z, bbox.max.x, 256, bbox.max.z);
	float3 mid_point = asXZY(test_box.center().xz(), bbox.min.y + 2.0f);

	bool vis_changed = update();
	vector<int> temp;
	temp.reserve(256);
	IRect test_rect = (IRect)worldToScreen(bbox);
	const Grid &grid = m_map.m_grid;
	grid.findAll(temp, test_rect);

	vector<int> temp2;
	temp2.reserve(256);

	PodArray<int> overlaps(m_map.size());
	memset(overlaps.data(), 0, m_map.size() * sizeof(int));

	for(int i = 0; i < (int)temp.size(); i++) {
		const auto &object = grid[temp[i]];
		if(object.occluder_id == -1)
			continue;

		const OccluderMap::Occluder &occluder = m_map[object.occluder_id];
		int order = drawingOrder(object.bbox, bbox);
		if(order == 1)
			overlaps[object.occluder_id] = order;
	}

	for(int n = 0; n < (int)m_states.size(); n++) {
		bool is_overlapping = false;
		const OccluderMap::Occluder &occluder = m_map[n];

		if(overlaps[n] == 1) {
			FBox bbox_around(bbox.min - int3(16, 0, 16), bbox.max + int3(16, 0, 16));
			bbox_around.min.y = 0;
			bbox_around.max.y = Grid::max_height;

			temp2.clear();
			grid.findAll(temp2, bbox_around);
			FBox local_box = FBox::empty();

			for(int i = 0; i < (int)temp2.size(); i++) {
				const auto &object = grid[temp2[i]];
				if(object.occluder_id == n)
					local_box = local_box.isEmpty()? object.bbox : sum(local_box, object.bbox);
			}

			is_overlapping = local_box.min.y > mid_point.y;
		}

		if(is_overlapping != m_states[n].is_overlapping) {
			m_states[n].is_overlapping = is_overlapping;
			vis_changed = true;
		}
	}

	if(!vis_changed)
		return false;

	for(int n= 0; n < (int)m_states.size(); n++)
		m_states[n].is_visible = !m_states[n].is_overlapping;

//TODO: isUnder can be precomputed
	for(int n = 0; n < (int)m_states.size(); n++) {
		if(!m_states[n].is_visible)
			continue;

		for(int i = 0; i < (int)m_states.size(); i++)
			if(!m_states[i].is_visible && m_map.isUnder(i, n)) {
				m_states[n].is_visible = false;
				break;
			}
	}

	return true;
}

bool OccluderConfig::isVisible(int occluder_id) const {
	return occluder_id < 0 || occluder_id >= (int)m_states.size() || m_states[occluder_id].is_visible;
}

void OccluderConfig::setVisible(int occluder_id, bool is_visible) {
	if(occluder_id >= 0 && occluder_id < (int)m_states.size())
	   m_states[occluder_id].is_visible = is_visible;
}

