/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef UI_TILE_LIST_H
#define UI_TILE_LIST_H

#include "game/tile.h"

class TileGroup;

namespace ui
{

	namespace TileFilter {
		enum Type {
			all,
			floors,
			walls,
			roofs,
			objects,
			other,

			count,
		};

		const char **strings();
		bool test(const game::Tile *tile, int filter);
	};

	struct TileListModel: public RefCounter {
		virtual ~TileListModel() { }
		virtual int size() const = 0;
		virtual const game::Tile* get(int idx, int &group_id) const = 0;
	};
	typedef Ptr<TileListModel> PTileListModel;

	PTileListModel allTilesModel();
	PTileListModel groupedTilesModel(const TileGroup&, bool only_uniform);
	PTileListModel filteredTilesModel(PTileListModel, TileFilter::Type);


	class TileList
	{
	public:
		struct Entry {
			const game::Tile *tile;
			int2 pos, size;
			int group_id, model_id, group_size;
			mutable bool is_selected;
			
			bool operator<(const Entry&) const;
		};

		TileList(int max_width, int spacing);
		void setModel(PTileListModel);

		const Entry* find(int2 pos) const;
		void update(); // call every time model changes

		int m_max_width, m_spacing;
		int m_height;

		int size() const { return (int)m_entries.size(); }
		const Entry& operator[](int idx) const { return m_entries[idx]; }
		Entry& operator[](int idx) { return m_entries[idx]; }

	private:
		vector<Entry> m_entries;
		PTileListModel m_model;
	};

}


#endif
