#ifndef UI_TILE_MAP_EDITOR_H
#define UI_TILE_MAP_EDITOR_H

#include "tile_map.h"
#include "gfx/tile.h"
#include "ui/window.h"

class TileGroup;

namespace ui {

	class TileMapEditor: public ui::Window
	{
	public:
		TileMapEditor(IRect rect);

		void setTileMap(TileMap*);
		void setTileGroup(const TileGroup *tile_group) { m_tile_group = tile_group; }
		void setNewTile(const gfx::Tile *tile) { m_new_tile = tile; }
			
		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);

		static void drawGrid(const IBox &box, int2 nodeSize, int y);

		//TODO: naming
		enum Mode {
			mSelecting,
			mPlacing,
			mPlacingRandom,
			mAutoFilling,

			mCount,
		} m_mode;

	private:
		TileMap *m_tile_map;
		const TileGroup *m_tile_group;
		const gfx::Tile *m_new_tile;

		void drawBoxHelpers(const IBox &box) const;
		IBox computeCursor(int2 start, int2 end) const;
		void clampViewPos();
		
		int m_cursor_height, m_grid_height;
		IBox m_selection;
		int2 m_view_pos;

		int2 m_grid_size;
		bool m_show_grid, m_is_selecting;
	};

	typedef Ptr<TileMapEditor> PTileMapEditor;

}


#endif

