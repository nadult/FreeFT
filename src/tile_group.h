#ifndef TILE_GROUP_H
#define TILE_GROUP_H

#include "gfx/tile.h"


class FloorTileGroup {
public:	
	struct Entry {
		const gfx::Tile *m_tile;
		int m_group_id;
		float m_weight;
	};

	struct Group {
		Group();

		enum { sideCount = 8 };
		int m_entry_count;
		int m_side_surf[sideCount];
	};

	void addEntry(const gfx::Tile*);
	int findEntry(const gfx::Tile*) const;

	void setEntryGroup(int entry_id, int group_id);
	void removeEntry(int entry_id);

	// an entry always has some group assigned
	int entryGroup(int entry_id) const { return m_entries[entry_id].m_group_id; }
	const gfx::Tile *entryTile(int entry_id) const { return m_entries[entry_id].m_tile; }

	void clear();

	void setGroupSurface(int group_id, int side, int surface) { m_groups[group_id].m_side_surf[side] = surface; }
	int  groupSurface(int group_id, int side) const { return m_groups[group_id].m_side_surf[side]; }

	int entryCount() const { return (int)m_entries.size(); }
	int groupCount() const { return (int)m_groups.size(); }
	int groupEntryCount(int group_id) const { return m_groups[group_id].m_entry_count; }

	void saveToXML(XMLDocument&) const;
	void loadFromXML(const XMLDocument&, const vector<gfx::Tile>&);

protected:
	void decGroupEntryCount(int group_id);

	vector<Entry> m_entries;
	vector<Group> m_groups;
	friend class FloorTileGroupEditor;
};




#endif
