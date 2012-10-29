#ifndef TILE_MAP_EDITOR_H
#define TILE_MAP_EDITOR_H

#include "tile_map.h"
#include "gfx/tile.h"
#include "ui/window.h"

class FloorTileGroup;

class TileMapCursor
{
public:

};

class TileMapEditor: public ui::Window
{
public:
	TileMapEditor(IRect rect);

	void setTileMap(TileMap*);
	void setTileGroup(const FloorTileGroup *tile_group) { m_tile_group = tile_group; }
	void setNewTile(const gfx::Tile *tile) { m_new_tile = tile; }
		
	virtual void drawContents() const;
	virtual void onInput(int2 mouse_pos);
	virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final);

	static void drawGrid(const IBox &box, int2 nodeSize, int y);

private:
	TileMap *m_tile_map;
	const FloorTileGroup *m_tile_group;
	const gfx::Tile *m_new_tile;

	int3 m_cursor_pos;	
	IBox m_selection;
	int2 m_view_pos;

	int2 m_grid_size;
	bool m_show_grid, m_is_selecting;
};



#endif

