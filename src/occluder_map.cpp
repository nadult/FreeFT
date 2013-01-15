/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "occluder_map.h"
#include "sys/xml.h"
#include <map>


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
		box.max += float3(1, 1, 1);
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
	
void OccluderMap::loadFromXML(const XMLDocument &doc) {
	clear();

	for(int n = 0; n < m_grid.size(); n++)
		if(m_grid[n].ptr)
			ASSERT(m_grid[n].occluder_id == -1);

	XMLNode main_node = doc.child("occluders");

	if(main_node) {
		XMLNode occluder_node = main_node.child("occluder");

		while(occluder_node) {
			XMLNode object_node = occluder_node.child("o");
			int occluder_id = size();
			m_occluders.push_back(Occluder());
			Occluder &occluder = m_occluders.back();
			
			int object_count = occluder_node.intAttrib("object_count");
			ASSERT(object_count < m_grid.size() && object_count > 0);
			int idx = 0;
			occluder.objects.resize(object_count);

			while(object_node) {
				int object_id = atoi(object_node.value());
				auto &object = m_grid[object_id];
				ASSERT(object.occluder_id == -1);
				object.occluder_id = occluder_id;
				occluder.bbox = idx == 0? object.bbox : sum(occluder.bbox, object.bbox);
				occluder.objects[idx++] = object_id;
				object_node = object_node.sibling("o");
			}
			
			occluder_node = occluder_node.sibling("occluder");
		}
	}
}

void OccluderMap::saveToXML(const PodArray<int> &tile_ids, XMLDocument &doc) const {
	//TODO: there are invalid objects in the grid, adjust object id accordingly
	XMLNode main_node = doc.addChild("occluders");
	for(int n = 0; n < (int)m_occluders.size(); n++) {
		const Occluder &occluder = m_occluders[n];

		XMLNode occluder_node = main_node.addChild("occluder");
		occluder_node.addAttrib("object_count", (int)occluder.objects.size());

		for(int o = 0; o < (int)occluder.objects.size(); o++) {
			char text[100];
			sprintf(text, "%d", tile_ids[occluder.objects[o]]);
			occluder_node.addChild("o", doc.own(text));
		}
	}
}

bool OccluderMap::updateVisibility(const FBox &bbox) {
	//TODO: hiding when close to a door/window
	FBox test_box(bbox.min.x, bbox.min.y + 1.0f, bbox.min.z, bbox.max.x, 256, bbox.max.z);
	float3 mid_point = asXZY(test_box.center().xz(), bbox.min.y + 1.0f);

	bool vis_changed = false;

	for(int n = 0; n < size(); n++) {
		bool is_overlapping = false;
		Occluder &occluder = m_occluders[n];

		if(!occluder.objects.empty()) {
			FBox rbox = m_grid[occluder.objects[0]].bbox;
			is_overlapping = areOverlapping(occluder.bbox, test_box) && mid_point.y < rbox.min.y;
		}

		if(is_overlapping != occluder.is_overlapping) {
			occluder.is_overlapping = is_overlapping;
			occluder.is_visible = !occluder.is_overlapping;
			vis_changed = true;
		}
	}

	if(!vis_changed)
		return false;

//TODO: isUnder can be precomputed
	for(int n = 0; n < size(); n++) {
		if(!m_occluders[n].is_visible)
			continue;

		for(int i = 0; i < size(); i++)
			if(!m_occluders[i].is_visible && isUnder(i, n)) {
				m_occluders[n].is_visible = false;
				break;
			}
	}
	return true;
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
