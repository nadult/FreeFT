/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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
