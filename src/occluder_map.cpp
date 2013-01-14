#include "occluder_map.h"
#include "sys/xml.h"
#include <map>


OccluderMap::OccluderMap(Grid *grid) :m_grid(grid) { DASSERT(grid); }

int OccluderMap::addOccluder(int representative_id) {
	DASSERT(representative_id >= 0 && representative_id < m_grid->size());
	auto &representative = (*m_grid)[representative_id];
	
	if(representative.occluder_id != -1)
		return representative.occluder_id;

	//TODO: something here is FUCKED UP; make occluder non-reusable
	int occluder_id = -1;
	for(int n = 0; n < (int)m_occluders.size(); n++)
		if(m_occluders[n].objects.empty()) {
			occluder_id = n;
			break;
		}

	if(occluder_id == -1) {
		occluder_id = (int)m_occluders.size();
		DASSERT(occluder_id < max_occluders);
		m_occluders.push_back(Occluder());
	}

	Occluder &occluder = m_occluders[occluder_id];
	vector<int> objects, temp;
	objects.reserve(1024);
	temp.reserve(128);

	representative.occluder_id = occluder_id;
	objects.push_back(representative_id);
	int min_height = representative.bbox.min.y;

	while(!objects.empty()) {
		int object_id = objects.back();
		objects.pop_back();
		occluder.objects.push_back(object_id);

		FBox box = (*m_grid)[object_id].bbox;
		box.min -= float3(1, 0, 1);
		box.max += float3(1, 1, 1);
		box.min.y = max(box.min.y, (float)min_height);
	
		temp.clear();
		m_grid->findAll(temp, box, object_id);

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

void OccluderMap::removeOccluder(int occluder_id) {
	DASSERT(occluder_id >= 0 && occluder_id < (int)m_occluders.size());
	for(int n = 0; n < (int)m_occluders.size(); n++)
		if(m_occluders[n].parent_id == occluder_id)
			m_occluders[n].parent_id = m_occluders[occluder_id].parent_id;

	m_occluders[occluder_id] = Occluder();
	for(int n = 0; n < m_grid->size(); n++) {
		auto &obj = (*m_grid)[n];
		if(obj.ptr) {
			if(obj.occluder_id == occluder_id)
				obj.occluder_id = -1;
			if(obj.occluded_by == occluder_id)
				obj.occluded_by = -1;
		}
	}
}

void OccluderMap::clear() {
	for(int n = 0; n < (int)m_occluders.size(); n++)
		removeOccluder(n);
	m_occluders.clear();
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
			while(object_node) {
				int id = atoi(object_node.value());
				auto &object = (*m_grid)[id];
				ASSERT(object.occluder_id == -1);
				object.occluder_id = (int)m_occluders.size();
				object_node = object_node.sibling("o");
			}
			
			m_occluders.push_back(Occluder());
			Occluder &occluder = m_occluders.back();
			occluder.parent_id = occluder_node.intAttrib("parent");

			occluder_node = occluder_node.sibling("occluder");
		}
	}
}

void OccluderMap::saveToXML(const PodArray<int> &tile_ids, XMLDocument &doc) const {
	//TODO: dont save empty occluders
	//TODO: there are invalid objects in the grid, adjust object id accordingly
	XMLNode main_node = doc.addChild("occluders");
	for(int n = 0; n < (int)m_occluders.size(); n++) {
		const Occluder &occluder = m_occluders[n];

		XMLNode occluder_node = main_node.addChild("occluder");
		occluder_node.addAttrib("parent", occluder.parent_id);

		for(int o = 0; o < (int)occluder.objects.size(); o++) {
			char text[100];
			sprintf(text, "%d", tile_ids[occluder.objects[o]]);
			occluder_node.addChild("o", doc.own(text));
		}
	}
}


