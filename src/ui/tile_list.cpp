#include "ui/tile_list.h"
#include <algorithm>

namespace ui
{

	AllTilesModel::AllTilesModel() {
		gfx::Tile::mgr.iterateOver( [&](const string&, const gfx::Tile &tile) { m_tiles.push_back(&tile); } );
	}


	bool TileList::Entry::operator<(const TileList::Entry &rhs) const {
		return group_id == rhs.group_id? tile->name < rhs.tile->name : group_id < rhs.group_id;
	}

	TileList::TileList(int max_width, int spacing)
		:m_max_width(max_width), m_spacing(spacing), m_height(0), m_model(nullptr) { }

	void TileList::setModel(PTileListModel model) {
		m_model = model;
		update();
	}

	const TileList::Entry* TileList::find(int2 pos) const {
		for(int n = 0; n < (int)m_entries.size(); n++) {
			const Entry &entry = m_entries[n];

			if(IRect(entry.pos, entry.pos + entry.size).isInside(pos))
				return &m_entries[n];
		}

		return nullptr;
	}

	void TileList::update() {
		if(!m_model) {
			m_entries.clear();
			return;
		}

		m_entries.resize(m_model->size());
		for(int n = 0, count = m_model->size(); n < count; n++) {
			Entry &entry = m_entries[n];
			entry.group_id = -1;
			entry.tile = m_model->get(n, entry.group_id);
			entry.is_selected = false;
			entry.model_id = n;
			entry.size = entry.tile->rect().size();
		}
		std::stable_sort(m_entries.begin(), m_entries.end());

		m_entries[0].group_size = 1;
		for(int n = 1; n < (int)m_entries.size(); n++) {
			Entry &cur = m_entries[n], &prev = m_entries[n - 1];
			cur.group_size = cur.group_id == prev.group_id? prev.group_size + 1 : 1;
		}
		for(int n = (int)m_entries.size() - 2; n >= 0; n--) {
			Entry &cur = m_entries[n], &prev = m_entries[n + 1];
			if(cur.group_id == prev.group_id)
				cur.group_size = prev.group_size;
		}

		// TODO: jesus, this one call makes the obj file 37KB bigger :(
		std::stable_sort(m_entries.begin(), m_entries.end(),
				[](const Entry &a, const Entry &b) { return a.group_size > b.group_size; } );

		int2 cur_pos(0, 0);
		int cur_height = 0;

		for(int e = 0; e < (int)m_entries.size(); e++) {
			Entry &entry = m_entries[e];
			entry.pos = cur_pos;
			cur_pos.x += m_spacing + entry.size.x;
			cur_height = max(cur_height, entry.size.y);
			int next_width = e + 1 < (int)m_entries.size()? m_entries[e + 1].size.x : 0;
			int next_group = e + 1 < (int)m_entries.size()? m_entries[e + 1].group_id : entry.group_id;

			if(next_width + cur_pos.x > m_max_width || (next_group != entry.group_id && entry.group_size > 1)) {
				cur_pos.x = 0;
				cur_pos.y += m_spacing + cur_height;
				cur_height = 0;
			}
		}

		m_height = cur_pos.y + cur_height;
	}

}
