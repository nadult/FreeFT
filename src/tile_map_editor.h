#ifndef TILE_MAP_EDITOR_H
#define TILE_MAP_EDITOR_H

#include "tile_map.h"
#include "gfx/tile.h"

class FloorTileGroup;

class TileMapCursor
{
public:

};

class TileMapEditor
{
public:
	TileMapEditor(int2 res);

	void loop(const gfx::Tile *new_tile);
	void draw(const gfx::Tile *new_tile);
	void setTileMap(TileMap*);
	void setTileGroup(const FloorTileGroup *tile_group) { m_tile_group = tile_group; }

	static void drawGrid(const IBox &box, int2 nodeSize, int y);

private:
	TileMap *m_tile_map;
	const FloorTileGroup *m_tile_group;
	int m_selected_tile;
	
	IRect m_view;
	IBox m_selection;
	int3 m_click_pos, m_world_pos;
	int2 m_grid_size;
	bool m_show_grid;
};



#endif

