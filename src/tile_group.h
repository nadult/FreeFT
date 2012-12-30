#ifndef TILE_GROUP_H
#define TILE_GROUP_H

#include "gfx/tile.h"


class TileGroup {
public:	
	struct Entry {
		const gfx::Tile *tile;
		int group_id;
		bool is_dirty; // like a floor tile with rubble; dirty tiles shouldnt be too dense on the map
	};

	struct Group {
		Group();

		enum { side_count = 8 };
		int m_entry_count;
		int m_side_surf[side_count]; // order: CCW starting from '1' on keypad
		static int3 s_side_offsets[side_count];
	};

	void addEntry(const gfx::Tile*);
	int findEntry(const gfx::Tile*) const;

	bool isValidEntryId(int id) const { return id >= 0 && id < entryCount(); }
	bool isValidEntryId(int id, const gfx::Tile *tile) const { return isValidEntryId(id) && entryTile(id) == tile; }

	void setEntryGroup(int entry_id, int group_id);
	void removeEntry(int entry_id);

	// an entry always has some group assigned
	int entryGroup(int entry_id) const { return m_entries[entry_id].group_id; }
	const gfx::Tile *entryTile(int entry_id) const { return m_entries[entry_id].tile; }

	bool isEntryDirty(int entry_id) const { return m_entries[entry_id].is_dirty; }
	void setEntryDirty(int entry_id, bool is_dirty) { m_entries[entry_id].is_dirty = is_dirty; }

	void clear();

	void setGroupSurface(int group_id, int side, int surface) { m_groups[group_id].m_side_surf[side] = surface; }
	int  groupSurface(int group_id, int side) const { return m_groups[group_id].m_side_surf[side]; }
	const int* groupSurface(int group_id) const { return m_groups[group_id].m_side_surf; }
	bool isGroupSurfaceUniform(int group_id) const;

	int entryCount() const { return (int)m_entries.size(); }
	int groupCount() const { return (int)m_groups.size(); }
	int groupEntryCount(int group_id) const { return m_groups[group_id].m_entry_count; }

	void saveToXML(XMLDocument&) const;
	void loadFromXML(const XMLDocument&);

protected:
	void decGroupEntryCount(int group_id);

	vector<Entry> m_entries;
	vector<Group> m_groups;
	friend class TileGroupEditor;
};




#endif
