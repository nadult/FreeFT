#ifndef TILE_GROUP_EDITOR_H
#define TILE_GROUP_EDITOR_H

#include "base.h"
#include "gfx/tile.h"
#include "gfx/font.h"
#include "gfx/device.h"
#include "ui/window.h"

class FloorTileGroup;

class FloorTileGroupEditor: public ui::Window {
public:
	FloorTileGroupEditor(IRect rect);

	virtual void onInput(int2 mouse_pos);
	virtual void drawContents() const;

	void setTarget(FloorTileGroup* tile_group);

protected:
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
