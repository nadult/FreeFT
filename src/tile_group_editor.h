#ifndef TILE_GROUP_EDITOR_H
#define TILE_GROUP_EDITOR_H

#include "base.h"
#include "gfx/tile.h"
#include "gfx/font.h"
#include "gfx/device.h"

class TileGroup;
class FloorTileGroup;

class TileGroupEditor {
public:
	TileGroupEditor(int2 res);

	void loop();

	void setSource(const vector<gfx::Tile> *tiles);
	void setTarget(TileGroup* tile_group);
	int tileCount() const;
	const gfx::Tile *getTile(int idx) const;

	const char *title() const;

protected:
	const vector<gfx::Tile> *m_tiles;
	TileGroup *m_tile_group;

	gfx::PFont m_font;
	gfx::PTexture m_font_texture;
	IRect m_view;

	int m_offset[3];
	const gfx::Tile *m_selected_tile;
	int m_selected_match_id;

	enum {
		mAddRemove, // add / remove tiles to / from group
		mModify, // modify tiles relations
	} m_mode;
};

class FloorTileGroupEditor {
public:
	FloorTileGroupEditor(int2 res);

	void loop();

	void setSource(const vector<gfx::Tile> *tiles);
	void setTarget(FloorTileGroup* tile_group);
	int tileCount() const;
	const gfx::Tile *getTile(int idx) const;

	const char *title() const;

protected:
	const vector<gfx::Tile> *m_tiles;
	FloorTileGroup *m_tile_group;

	gfx::PFont m_font;
	gfx::PTexture m_font_texture;
	IRect m_view;

	int m_offset[3];
	const gfx::Tile *m_selected_tile;
	int m_selected_match_id;
	int m_selected_surface_id;
	int m_select_mode;
	enum {
		mAddRemove, // add / remove tiles to / from group
		mModify, // modify tiles relations
	} m_mode;
};



#endif
