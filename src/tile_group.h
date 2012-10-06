#ifndef TILE_GROUP_H
#define TILE_GROUP_H

#include "gfx/tile.h"


class TileGroup {
public:	
	struct Match {
		int m_tile_id;
		int3 m_offset; 
	};

	struct Entry {
		const gfx::Tile *m_tile;
		int m_parent_id;
		vector<Match> m_matches;
	};

	void addEntry(const gfx::Tile* tile);
	void removeEntry(int idx);
	void clear();

	void addMatch(int tile_idx, int matched_tile_idx, int3 offset);
	void removeMatch(int tile_idx, int matched_tile_idx);
	void clearMatches(int tile_idx);

	// -1 means no parent
	void setParentTile(int tile_id, int parent_id);

	const Entry& operator[](int idx) const { return m_entries[idx]; }
	int size() const { return (int)m_entries.size(); }
	vector<Match>& getMatches(int tile_idx);
	const vector<Match>& getMatches(int tile_idx) const;

protected:
	vector<Entry> m_entries;
};



#endif
