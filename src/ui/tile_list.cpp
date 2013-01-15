/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "ui/tile_list.h"
#include "editor/tile_group.h"
#include "game/tile.h"
#include <algorithm>

using namespace gfx;
using namespace game;

namespace ui
{

	namespace TileFilter {

		static const char *s_strings[TileFilter::count] = {
			"all",
			"floors",
			"walls",
			"roofs",
			"objects",
			"other",
		};

		static int s_filters[TileFilter::count] = {
			-1,
			TileId::floor,
			TileId::wall,
			TileId::roof,
			TileId::object,
			TileId::unknown,
		};

		const char **strings() {
			return s_strings;
		}

		bool filter(const Tile *tile, int filter) {
			DASSERT(tile);
			DASSERT(filter < TileFilter::count && filter >= 0);
			return s_filters[filter] == -1 || tile->type() == s_filters[filter];
		}
	}

	namespace {

		struct VectorBasedModel: public TileListModel {
			VectorBasedModel(const vector<const Tile*> &tiles) :m_tiles(tiles) { }

			int size() const { return (int)m_tiles.size(); }
			const Tile* get(int idx, int&) const { return m_tiles[idx]; }

		protected:
			vector<const Tile*> m_tiles;
		};

		struct FilteredModel: public TileListModel {
			FilteredModel(PTileListModel model, bool (*filter)(const Tile*, int), int param)
								:m_sub_model(model) {
				for(int n = 0; n < model->size(); n++)	{
					int group = 0;
					const Tile *tile = model->get(n, group);
					if(filter(tile, param))
						m_indices.push_back(n);
				}
			}
			
			int size() const { return (int)m_indices.size(); }
			const Tile* get(int id, int &group) const { return m_sub_model->get(m_indices[id], group); }

		protected:
			PTileListModel m_sub_model;
			vector<int> m_indices;
		};

	}

	PTileListModel allTilesModel() {
		vector<const Tile*> tiles;
		Tile::mgr.iterateOver( [&](const string&, const Tile &tile) { tiles.push_back(&tile); } );
		return new VectorBasedModel(tiles);
	}

	PTileListModel groupedTilesModel(const TileGroup &tile_group, bool only_uniform) {
		vector<const Tile*> tiles(tile_group.groupCount());
		for(int n = 0; n < tile_group.entryCount(); n++) {
			int group_id = tile_group.entryGroup(n);
			if(!tiles[group_id] && (!only_uniform || tile_group.isGroupSurfaceUniform(group_id)))
				tiles[group_id] = tile_group.entryTile(n);
		}
		tiles.resize(remove(tiles.begin(), tiles.end(), nullptr) - tiles.begin());
		return new VectorBasedModel(tiles);
	}

	PTileListModel filteredTilesModel(PTileListModel model, TileFilter::Type param) {
		return new FilteredModel(model, TileFilter::filter, param);
	}

	bool TileList::Entry::operator<(const TileList::Entry &rhs) const {
		return group_id == rhs.group_id? strcmp(tile->name(), rhs.tile->name()) < 0 : group_id < rhs.group_id;
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

		if(!m_entries.empty())
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
