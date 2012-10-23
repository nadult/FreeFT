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
	virtual bool onMouseClick(int2 pos, int key);
	virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final);

	void setSource(const vector<gfx::Tile> *tiles);
	void setSource(const TileGroup* tile_group);

	int tileCount() const					{ return m_tiles? (int)m_tiles->size() : 0; }
	const gfx::Tile *getTile(int idx) const	{ return m_tiles? &(*m_tiles)[idx] : nullptr; }

protected:
	ui::TileList m_tile_list;
	const vector<gfx::Tile> *m_tiles;
	const TileGroup *m_tile_group;

	int2 m_offset;
	const ui::TileList::Entry *m_selection;
};


#endif
