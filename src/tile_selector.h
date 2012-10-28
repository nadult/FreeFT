#ifndef TILE_SELECTOR_H
#define TILE_SELECTOR_H

#include "base.h"
#include "gfx/tile.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

class TileSelector: public ui::Window {
public:
	TileSelector(IRect rect);

	virtual void drawContents() const;

	virtual void onInput(int2 mouse_pos); 
	virtual bool onMouseClick(int2 pos, int key, bool up);
	virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final);

	void updateTiles();
	const gfx::Tile* selection() const { return m_selection? m_selection->m_tile : nullptr; }

protected:
	ui::TileList m_tile_list;

	int2 m_offset;
	const ui::TileList::Entry *m_selection;
};


#endif
