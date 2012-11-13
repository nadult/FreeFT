#ifndef UI_TILE_SELECTOR_H
#define UI_TILE_SELECTOR_H

#include "base.h"
#include "gfx/tile.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

namespace ui {

	class TileSelector: public Window {
	public:
		TileSelector(IRect rect);

		virtual void drawContents() const;
		virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final);

		void setModel(PTileListModel);
		void update(); // call every time model changes

		const gfx::Tile *selection() const { return m_selection? m_selection->tile : nullptr; }
		void setSelection(const gfx::Tile*);

	protected:
		TileList m_tile_list;
		const TileList::Entry *m_selection;
	};

	typedef Ptr<TileSelector> PTileSelector;

}

#endif
