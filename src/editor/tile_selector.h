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

#ifndef UI_TILE_SELECTOR_H
#define UI_TILE_SELECTOR_H

#include "base.h"
#include "game/tile.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

namespace ui {

	class TileSelector: public Window {
	public:
		TileSelector(IRect rect);

		virtual void drawContents() const;
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);

		void setModel(PTileListModel);
		void update(); // call every time model changes

		const game::Tile *selection() const { return m_selection? m_selection->tile : nullptr; }
		void setSelection(const game::Tile*);

	protected:
		TileList m_tile_list;
		const TileList::Entry *m_selection;
	};

	typedef Ptr<TileSelector> PTileSelector;

}

#endif
