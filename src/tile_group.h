#ifndef TILE_GROUP_H
#define TILE_GROUP_H

#include "gfx/tile.h"


class TileGroup {
public:	
	struct Match {
		int m_entry_id;
		int3 m_offset; 
	};

	struct Entry {
		const gfx::Tile* representative() const {
			DAssert(!m_tiles.empty());
			return m_tiles.front();
		}

		vector<const gfx::Tile*> m_tiles;
		vector<Match> m_matches;
	};

	void addTile(const gfx::Tile*);
	void removeTile(const gfx::Tile*);

	int findEntry(const gfx::Tile*) const;
	void removeEntry(int entry_id);
	void mergeEntries(int src_entry_id, int target_entry_id);

	void clear();

	void addMatch(int entry_id, int target_id, int3 offset);
	void removeMatch(int entry_id, int match_id);
	void clearMatches(int entry_id);

	const Entry& operator[](int idx) const { return m_entries[idx]; }
	int size() const { return (int)m_entries.size(); }

	void saveToXML(XMLDocument&) const;
	void loadFromXML(const XMLDocument&, const vector<gfx::Tile>&);

protected:
	vector<Entry> m_entries;
	friend class TileGroupEditor;
};



#endif