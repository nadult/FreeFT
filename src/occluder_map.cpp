#include "occluder_map.h"
#include "sys/xml.h"
#include <map>


OccluderMap::OccluderMap(Grid *grid) :m_grid(grid) { DASSERT(grid); }

int OccluderMap::addOccluder(int representative_id) {
	DASSERT(representative_id >= 0 && representative_id < m_grid->size());
	auto &representative = (*m_grid)[representative_id];
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

		FBox box = (*m_grid)[object_id].bbox;
		occluder.bbox = sum(occluder.bbox, box);

		box.min -= float3(1, 0, 1);
		box.max += float3(1, 1, 1);
		box.min.y = max(box.min.y, (float)min_height);
	
		temp.clear();
		m_grid->findAll(temp, box, object_id, 0);

		for(int n = 0; n < (int)temp.size(); n++) {
			auto &object = (*m_grid)[temp[n]];
			if(object.occluder_id != -1) {
				Occluder &nextocc = m_occluders[object.occluder_id];
				if(object.occluder_id != occluder_id && occluder.parent_id != object.occluder_id &&
						nextocc.parent_id == -1)
					nextocc.parent_id = occluder_id;
				continue;
			}
			object.occluder_id = occluder_id;
			objects.push_back(temp[n]);
		}
	}

	return occluder_id;
}

/* TODO: untested
void OccluderMap::updateParents() {
	for(int n = 0; n < size(); n++)
		m_occluders[n].parent_id = -1;
	vector<int> temp;
	temp.reserve(128);

	for(int n = 0; n < size(); n++) {
		const Occluder &occluder = m_occluders[n];
		for(int o = 0; o < (int)occluder.objects.size(); o++) {
			FBox box = (*m_grid)[occluder.objects[o]].bbox;
			box.min -= float3(1, 0, 1);
			box.max += float3(1, 1, 1);

			temp.clear();
			m_grid->findAll(temp, box);
			for(int i = 0; i < (int)temp.size(); i++) {
				const auto &object = (*m_grid)[temp[i]];
				if(object.occluder_id != -1 && object.occluder_id != n) {
					Occluder &other = m_occluders[object.occluder_id];
					if(other.parent_id == -1)
						other.parent_id = n;
				}
			}
		}
	}
}*/

void OccluderMap::removeOccluder(int occluder_id) {
	DASSERT(occluder_id >= 0 && occluder_id < (int)m_occluders.size());
#define FIX_INDEX(ref) { if(ref == occluder_id) ref = -1; else if(ref > occluder_id) ref--; }

	for(int n = 0; n < m_grid->size(); n++) {
		auto &object = (*m_grid)[n];
		if(!object.ptr)
			continue;

		FIX_INDEX(object.occluder_id);
		FIX_INDEX(object.occluded_by);
	}
	
	for(int n = 0; n < size(); n++)
		if(n != occluder_id)
			FIX_INDEX(m_occluders[n].parent_id);

#undef FIX_INDEX

	m_occluders.erase(m_occluders.begin() + occluder_id);
	//TODO: if removed occluder in the middle, parent_id info will be wrong
}

void OccluderMap::clear() {
	while(size() > 0)
		removeOccluder(size() - 1);
}
	
void OccluderMap::loadFromXML(const XMLDocument &doc) {
	clear();

	for(int n = 0; n < m_grid->size(); n++)
		if((*m_grid)[n].ptr)
			ASSERT((*m_grid)[n].occluder_id == -1);

	XMLNode main_node = doc.child("occluders");

	if(main_node) {
		XMLNode occluder_node = main_node.child("occluder");

		while(occluder_node) {
			XMLNode object_node = occluder_node.child("o");
			int occluder_id = size();
			m_occluders.push_back(Occluder());
			Occluder &occluder = m_occluders.back();
			
			occluder.parent_id = occluder_node.intAttrib("parent");
			int object_count = occluder_node.intAttrib("object_count");
			ASSERT(object_count < m_grid->size() && object_count > 0);
			int idx = 0;
			occluder.objects.resize(object_count);

			while(object_node) {
				int object_id = atoi(object_node.value());
				auto &object = (*m_grid)[object_id];
				ASSERT(object.occluder_id == -1);
				object.occluder_id = occluder_id;
				occluder.bbox = idx == 0? object.bbox : sum(occluder.bbox, object.bbox);
				occluder.objects[idx++] = object_id;
				object_node = object_node.sibling("o");
			}
			
			occluder_node = occluder_node.sibling("occluder");
		}
	}

	for(int n = 0; n < size(); n++) {
		int id = n, hcount = 0;
		while(id != -1) {
			id = m_occluders[id].parent_id;
			ASSERT(id == -1 || (id >= 0 && id < size()));
			hcount++;
			ASSERT(hcount <= size());
		}
	}
}

void OccluderMap::saveToXML(const PodArray<int> &tile_ids, XMLDocument &doc) const {
	//TODO: there are invalid objects in the grid, adjust object id accordingly
	XMLNode main_node = doc.addChild("occluders");
	for(int n = 0; n < (int)m_occluders.size(); n++) {
		const Occluder &occluder = m_occluders[n];

		XMLNode occluder_node = main_node.addChild("occluder");
		occluder_node.addAttrib("parent", occluder.parent_id);
		occluder_node.addAttrib("object_count", (int)occluder.objects.size());

		for(int o = 0; o < (int)occluder.objects.size(); o++) {
			char text[100];
			sprintf(text, "%d", tile_ids[occluder.objects[o]]);
			occluder_node.addChild("o", doc.own(text));
		}
	}
}

void OccluderMap::updateVisibility(const FBox &bbox) {
	//TODO: hiding when close to a door/window
	FBox test_box(bbox.min.x, bbox.min.y + 1.0f, bbox.min.z, bbox.max.x, 256, bbox.max.z);
	float3 mid_point = asXZY(test_box.center().xz(), bbox.min.y);

	for(int n = 0; n < size(); n++) {
		if(m_occluders[n].objects.empty())
			continue;

		FBox rbox = (*m_grid)[m_occluders[n].objects[0]].bbox;
		bool overlaps = areOverlapping(m_occluders[n].bbox, test_box);
		m_occluders[n].is_visible = !overlaps || mid_point.y >= rbox.min.y;
	}

	for(int n = 0; n < size(); n++) {
		int parent = m_occluders[n].parent_id;
		while(parent != -1) {
			if(!m_occluders[parent].is_visible)
				m_occluders[n].is_visible = false;
			parent = m_occluders[parent].parent_id;
		}
	}
}
