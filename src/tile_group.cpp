#include "tile_group.h"


void TileGroup::addTile(const gfx::Tile* tile) {
	DAssert(tile);

	Entry new_entry;
	new_entry.m_tiles.push_back(tile);
	m_entries.push_back(new_entry);
}

void TileGroup::removeTile(const gfx::Tile* tile) {
	for(int e = 0; e < size(); e++) {
		Entry &entry = m_entries[e];
		for(int t = 0; t < (int)entry.m_tiles.size(); t++) 
			if(entry.m_tiles[t] == tile) {
				entry.m_tiles[t] = entry.m_tiles.back();
				entry.m_tiles.pop_back();
				if(entry.m_tiles.empty())
					removeEntry(e);
				return;
			}
	}
}

int TileGroup::findEntry(const gfx::Tile *tile) const {
	for(int e = 0; e < size(); e++) {
		const Entry &entry = m_entries[e];
		for(int t = 0; t < (int)entry.m_tiles.size(); t++) 
			if(entry.m_tiles[t] == tile)
				return e;
	}

	return -1;
}

void TileGroup::removeEntry(int entry_id) {
	DAssert(entry_id < size() && entry_id >= 0);

	for(int n = 0; n < size(); n++) {
		Entry &entry = m_entries[n];
		if(n == entry_id)
			continue;

		for(int m = 0; m < (int)entry.m_matches.size(); m++) {
			Match &match = entry.m_matches[m];
			if(match.m_entry_id == entry_id) {
				entry.m_matches[m--] = entry.m_matches.back();
				entry.m_matches.pop_back();
			}
			else if(match.m_entry_id > entry_id)
				match.m_entry_id--;
		}
	}

	m_entries.erase(m_entries.begin() + entry_id);
}

void TileGroup::mergeEntries(int src_entry_id, int target_entry_id) {
	DAssert(src_entry_id >= 0 && target_entry_id >= 0 && src_entry_id < size() && target_entry_id < size());
	if(src_entry_id == target_entry_id)
		return;

	Entry &target = m_entries[target_entry_id];
	Entry &source = m_entries[src_entry_id];

	target.m_tiles.insert(target.m_tiles.end(), source.m_tiles.begin(), source.m_tiles.end());
	target.m_matches.insert(target.m_matches.end(), source.m_matches.begin(), source.m_matches.end());

	removeEntry(src_entry_id);
}

void TileGroup::addMatch(int entry_id, int matched_entry_id, int3 offset) {
	DAssert(entry_id < size() && matched_entry_id < size() && entry_id >= 0 && matched_entry_id >= 0);	
	m_entries[entry_id].m_matches.push_back(Match{matched_entry_id, offset});
}
	
void TileGroup::removeMatch(int entry_id, int matched_entry_id) {
	DAssert(entry_id < size() && matched_entry_id < size() && entry_id >= 0 && matched_entry_id >= 0);	

	vector<Match> &matches = m_entries[entry_id].m_matches;
	for(int n = 0; n < (int)matches.size(); n++)
		if(matches[n].m_entry_id == matched_entry_id) {
			matches[n--] = matches.back();
			matches.pop_back();
		}
}

void TileGroup::clearMatches(int entry_id) {
	DAssert(entry_id < size() && entry_id >= 0);
	m_entries[entry_id].m_matches.clear();
}

void TileGroup::clear() {
	m_entries.clear();
}
