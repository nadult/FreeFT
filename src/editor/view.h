/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "base.h"

namespace ui {

	class View {
	public:
		View(game::TileMap&, const int2 &view_size);

		void drawGrid() const;
		void update();

		void setGridHeight(int new_height) { m_height = new_height; }
		int gridHeight() const { return m_height; }
		int cellSize() const { return m_cell_size; }
		bool isGridVisible() const { return m_is_visible; }

		const int2 &pos() const { return m_view_pos; }
		void updateVisibility(int cursor_height = 0);

	private:
		game::TileMap &m_tile_map;
		int m_height, m_cell_size;
		bool m_is_visible;
		int2 m_view_pos;
		const int2 m_view_size;
	};

	typedef unique_ptr<View> PView;

}

#endif
