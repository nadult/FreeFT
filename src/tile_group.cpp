#include "tile_group.h"


void TileGroup::addEntry(const gfx::Tile* tile) {
	Entry new_entry;
	new_entry.m_tile = tile;
	new_entry.m_parent_id = -1;
	m_entries.push_back(new_entry);
}

void TileGroup::removeEntry(int idx) {
	DAssert(idx < size() && idx >= 0);
	int new_parent = -1;

	for(int n = 0; n < size(); n++)
		if(m_entries[n].m_parent_id == idx) {
			new_parent = n;
			break;
		}

	for(int n = 0; n < size(); n++) {
		Entry &entry = m_entries[n];
		if(n == idx)
			continue;
		if(entry.m_parent_id == idx) {
			if(new_parent == n) {
				entry.m_matches = m_entries[idx].m_matches;
				entry.m_parent_id = -1;
			}
			else
				entry.m_parent_id = new_parent;
		}

		for(int m = 0; m < (int)entry.m_matches.size(); m++) {
			Match &match = entry.m_matches[m];
			if(match.m_tile_id == idx) {
				entry.m_matches[m--] = entry.m_matches.back();
				entry.m_matches.pop_back();
			}
			else if(match.m_tile_id > idx)
				match.m_tile_id--;
		}
	}

	m_entries.erase(m_entries.begin() + idx);
}

void TileGroup::addMatch(int tile_idx, int matched_tile_idx, int3 offset) {
	DAssert(tile_idx < size() && matched_tile_idx < size() && tile_idx >= 0 && matched_tile_idx >= 0);	
	getMatches(tile_idx).push_back(Match{matched_tile_idx, offset});
}
	
void TileGroup::removeMatch(int tile_idx, int matched_tile_idx) {
	DAssert(tile_idx < size() && matched_tile_idx < size() && tile_idx >= 0 && matched_tile_idx >= 0);	

	vector<Match> &matches = getMatches(tile_idx);
	for(int n = 0; n < (int)matches.size(); n++)
		if(matches[n].m_tile_id == matched_tile_idx) {
			matches[n--] = matches.back();
			matches.pop_back();
		}
}

void TileGroup::clearMatches(int tile_idx) {
	DAssert(tile_idx < size() && tile_idx >= 0);
	m_entries[tile_idx].m_matches.clear();
}

void TileGroup::setParentTile(int tile_idx, int parent_id) {
	DAssert(tile_idx < size() && tile_idx >= 0 && parent_id < size() && parent_id >= 0 && parent_id != tile_idx);
	const Entry& parent = m_entries[parent_id];
	m_entries[tile_idx].m_parent_id = parent.m_parent_id == -1? parent_id : parent.m_parent_id;
	m_entries[tile_idx].m_matches.clear(); //TODO: join to parent?
}

void TileGroup::clear() {
	m_entries.clear();
}
	
vector<TileGroup::Match>& TileGroup::getMatches(int tile_idx) {
	DAssert(tile_idx < size() && tile_idx >= 0);
	Entry *entry = &m_entries[tile_idx];
	while(entry->m_parent_id != -1)
		entry = &m_entries[entry->m_parent_id];
	return entry->m_matches;
}

const vector<TileGroup::Match>& TileGroup::getMatches(int tile_idx) const {
	return const_cast<TileGroup*>(this)->getMatches(tile_idx);
}
