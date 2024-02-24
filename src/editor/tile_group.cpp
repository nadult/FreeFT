// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/tile_group.h"

using namespace game;

int3 TileGroup::Group::s_side_offsets[TileGroup::Group::side_count] = {
	{0, 0, 1}, {1, 0, 1}, {1, 0, 0}, {1, 0, -1}, {0, 0, -1}, {-1, 0, -1}, {-1, 0, 0}, {-1, 0, 1}};

TileGroup::Group::Group() : m_entry_count(0) {
	for(int n = 0; n < side_count; n++)
		m_side_surf[n] = -1;
}

void TileGroup::addEntry(const Tile *tile) {
	DASSERT(tile);

	Entry new_entry;
	new_entry.tile = tile;
	new_entry.group_id = groupCount();
	new_entry.is_dirty = false;
	m_entries.push_back(new_entry);
	m_groups.push_back(Group());
	m_groups.back().m_entry_count++;
}

int TileGroup::findEntry(const Tile *tile) const {
	for(int n = 0; n < entryCount(); n++)
		if(m_entries[n].tile == tile)
			return n;
	return -1;
}

void TileGroup::decGroupEntryCount(int group_id) {
	DASSERT(group_id >= 0 && group_id < groupCount());

	if(!--m_groups[group_id].m_entry_count) {
		m_groups[group_id] = m_groups.back();
		m_groups.pop_back();

		for(int n = 0; n < entryCount(); n++)
			if(m_entries[n].group_id == groupCount())
				m_entries[n].group_id = group_id;
	}
}

void TileGroup::setEntryGroup(int entry_id, int group_id) {
	DASSERT(entry_id >= 0 && entry_id < entryCount() && group_id >= 0 && group_id <= groupCount());

	int last_group = m_entries[entry_id].group_id;
	if(group_id == last_group)
		return;
	m_entries[entry_id].group_id = group_id;

	if(group_id == groupCount())
		m_groups.push_back(Group());
	m_groups[group_id].m_entry_count++;
	decGroupEntryCount(last_group);
}

void TileGroup::removeEntry(int entry_id) {
	DASSERT(entry_id >= 0 && entry_id < entryCount());

	int group_id = m_entries[entry_id].group_id;
	m_entries[entry_id] = m_entries.back();
	m_entries.pop_back();
	decGroupEntryCount(group_id);
}

bool TileGroup::isGroupSurfaceUniform(int group_id) const {
	const Group &group = m_groups[group_id];
	for(int n = 1; n < Group::side_count; n++)
		if(group.m_side_surf[n] != group.m_side_surf[0])
			return false;
	return true;
}

void TileGroup::clear() {
	m_groups.clear();
	m_entries.clear();
}

void TileGroup::saveToXML(XmlDocument &doc) const {
	for(int n = 0; n < entryCount(); n++) {
		const Entry &entry = m_entries[n];
		auto entry_node = doc.addChild("entry");
		entry_node.addAttrib("tile", doc.own(entry.tile->resourceName()));
		entry_node.addAttrib("group_id", entry.group_id);
		entry_node.addAttrib("is_dirty", (int)entry.is_dirty);
	}

	Str side_names[Group::side_count];
	for(int s = 0; s < Group::side_count; s++) {
		char name[32];
		snprintf(name, sizeof(name), "side_surf_%d", s);
		side_names[s] = doc.own(name);
	}

	for(int n = 0; n < groupCount(); n++) {
		const Group &group = m_groups[n];
		auto group_node = doc.addChild("group");

		for(int s = 0; s < Group::side_count; s++)
			group_node.addAttrib(side_names[s], group.m_side_surf[s]);
	}
}

void TileGroup::loadFromXML(const XmlDocument &doc) {
	clear();

	auto node = doc.child("entry");

	while(node) {
		Entry entry;
		entry.tile = &res::getTile(node.attrib("tile"));
		entry.group_id = node.attrib<int>("group_id");
		entry.is_dirty = node.attrib<int>("is_dirty") != 0;
		m_entries.push_back(entry);

		node = node.sibling("entry");
	}

	char side_names[Group::side_count][32];
	for(int s = 0; s < Group::side_count; s++)
		snprintf(side_names[s], sizeof(side_names[0]), "side_surf_%d", s);

	node = doc.child("group");
	while(node) {
		Group group;
		for(int s = 0; s < Group::side_count; s++)
			group.m_side_surf[s] = node.attrib<int>(side_names[s]);
		m_groups.push_back(group);
		node = node.sibling("group");
	}

	for(int n = 0; n < entryCount(); n++) {
		ASSERT(m_entries[n].group_id >= 0 && m_entries[n].group_id < groupCount());
		m_groups[m_entries[n].group_id].m_entry_count++;
	}
}
