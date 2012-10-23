#ifndef UI_TILE_LIST_H
#define UI_TILE_LIST_H

#include "gfx/tile.h"

namespace ui
{

	class TileList
	{
	public:
		struct Entry {
			const gfx::Tile *m_tile;
			int2 m_pos, m_size;
			int m_group;
			
			bool operator<(const Entry&) const;
		};

		TileList(int max_width, int spacing);
		const Entry* find(int2 pos) const;
		void add(const gfx::Tile *tile, int group = 0);
		void update();
		void clear();

		int m_max_width, m_spacing;
		int m_height;

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

		vector<Entry> m_entries;
	};

}


#endif
