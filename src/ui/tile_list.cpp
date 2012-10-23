#include "ui/tile_list.h"
#include <algorithm>

namespace ui
{

	bool TileList::Entry::operator<(const TileList::Entry &rhs) const {
		return m_group == rhs.m_group? m_tile->name < rhs.m_tile->name : m_group < rhs.m_group;
	}

	TileList::TileList(int max_width, int spacing) :m_max_width(max_width), m_spacing(spacing), m_height(0) { }

	const TileList::Entry* TileList::find(int2 pos) const {
		for(int n = 0; n < (int)m_entries.size(); n++) {
			const Entry &entry = m_entries[n];

			if(IRect(entry.m_pos, entry.m_pos + entry.m_size).IsInside(pos))
				return &m_entries[n];
		}

		return nullptr;
	}

	void TileList::add(const gfx::Tile *tile, int group) {
		DAssert(tile);
		m_entries.push_back(Entry{tile, int2(0, 0), tile->texture.Size(), group});
	}

	void TileList::clear() {
		m_entries.clear();
	}

	void TileList::update() {
		std::stable_sort(m_entries.begin(), m_entries.end());

		int2 cur_pos(0, 0);
		int cur_height = 0;

		for(int e = 0; e < (int)m_entries.size(); e++) {
			Entry &entry = m_entries[e];
			entry.m_pos = cur_pos;
			cur_pos.x += m_spacing + entry.m_size.x;
			cur_height = Max(cur_height, entry.m_size.y);
			int next_width = e + 1 < (int)m_entries.size()? m_entries[e + 1].m_size.x : 0;
			int next_group = e + 1 < (int)m_entries.size()? m_entries[e + 1].m_group : entry.m_group;

			if(next_width + cur_pos.x > m_max_width || next_group != entry.m_group) {
				cur_pos.x = 0;
				cur_pos.y += m_spacing + cur_height;
			}
		}

		m_height = cur_pos.y + cur_height;
	}

}
