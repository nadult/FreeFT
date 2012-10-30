#include "tile_group.h"
#include <cstring>

using namespace rapidxml;

int3 TileGroup::Group::s_side_offsets[TileGroup::Group::sideCount] = {
	{  0, 0,  1 },
	{  1, 0,  1 },
	{  1, 0,  0 },
	{  1, 0, -1 },
	{  0, 0, -1 },
	{ -1, 0, -1 },
	{ -1, 0,  0 },
	{ -1, 0,  1 } };

TileGroup::Group::Group() :m_entry_count(0) {
	for(int n = 0; n < sideCount; n++)
	   m_side_surf[n] = -1;	
}

void TileGroup::addEntry(const gfx::Tile *tile) {
	DAssert(tile);

	Entry new_entry;
	new_entry.m_tile = tile;
	new_entry.m_group_id = groupCount();
	m_entries.push_back(new_entry);
	m_groups.push_back(Group());
	m_groups.back().m_entry_count++;

}

int TileGroup::findEntry(const gfx::Tile *tile) const {
	for(int n = 0; n < entryCount(); n++)
		if(m_entries[n].m_tile == tile)
			return n;
	return -1;
}

void TileGroup::decGroupEntryCount(int group_id) {
	DAssert(group_id >= 0 && group_id < groupCount());

	if(!--m_groups[group_id].m_entry_count) {
		m_groups[group_id] = m_groups.back();
		m_groups.pop_back();

		for(int n = 0; n < entryCount(); n++)
			if(m_entries[n].m_group_id == groupCount())
				m_entries[n].m_group_id = group_id;
	}
}

void TileGroup::setEntryGroup(int entry_id, int group_id) {
	DAssert(entry_id >= 0 && entry_id < entryCount() && group_id >= 0 && group_id <= groupCount());

	int last_group = m_entries[entry_id].m_group_id;
	if(group_id == last_group)
		return;
	m_entries[entry_id].m_group_id = group_id;

	if(group_id == groupCount())
		m_groups.push_back(Group());
	m_groups[group_id].m_entry_count++;
	decGroupEntryCount(last_group);
}

void TileGroup::removeEntry(int entry_id) {
	DAssert(entry_id >= 0 && entry_id < entryCount());

	int group_id = m_entries[entry_id].m_group_id;
	m_entries[entry_id] = m_entries.back();
	m_entries.pop_back();
	decGroupEntryCount(group_id);
}

void TileGroup::clear() {
	m_groups.clear();
	m_entries.clear();
}

void TileGroup::saveToXML(XMLDocument &doc) const {
	for(int n = 0; n < entryCount(); n++) {
		const Entry &entry = m_entries[n];
		XMLNode *entry_node = doc.allocate_node(node_element, "entry");
		doc.append_node(entry_node);

		addAttribute(entry_node, "tile", entry.m_tile->name.c_str());
		addAttribute(entry_node, "group_id", entry.m_group_id);
		addAttribute(entry_node, "weight", entry.m_weight);
	}

	const char *side_names[Group::sideCount];
	for(int s = 0; s < Group::sideCount; s++) {
		char name[32];
		snprintf(name, sizeof(name), "side_surf_%d", s);
		side_names[s] = doc.allocate_string(name);
	}

	for(int n = 0; n < groupCount(); n++) {
		const Group &group = m_groups[n];
		XMLNode *group_node = doc.allocate_node(node_element, "group");
		doc.append_node(group_node);

		for(int s = 0; s < Group::sideCount; s++)
			addAttribute(group_node, side_names[s], group.m_side_surf[s]);
	}
}

void TileGroup::loadFromXML(const XMLDocument &doc) {
	clear();

	XMLNode *node = doc.first_node("entry");

	while(node) {
		Entry entry;
		Ptr<gfx::Tile> tile = gfx::Tile::mgr[getStringAttribute(node, "tile")];

		entry.m_tile = &*tile;
		entry.m_group_id = getIntAttribute(node, "group_id");
		entry.m_weight = getFloatAttribute(node, "weight");
		m_entries.push_back(entry);

		node = node->next_sibling("entry");
	}

	char side_names[Group::sideCount][32];
	for(int s = 0; s < Group::sideCount; s++)
		snprintf(side_names[s], sizeof(side_names[0]), "side_surf_%d", s);

	node = doc.first_node("group");
	while(node) {
		Group group;
		for(int s = 0; s < Group::sideCount; s++)
			group.m_side_surf[s] = getIntAttribute(node, side_names[s]);
		m_groups.push_back(group);
		node = node->next_sibling("group");
	}

	for(int n = 0; n < entryCount(); n++) {
		Assert(m_entries[n].m_group_id >= 0 && m_entries[n].m_group_id < groupCount());
		m_groups[m_entries[n].m_group_id].m_entry_count++;
	}
}


