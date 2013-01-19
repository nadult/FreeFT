/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */



#ifndef EDITOR_SNAPPING_GRID_H
#define EDITOR_SNAPPING_GRID_H

#include "base.h"


// TODO: Change this class to EditorView or something, it should also contain m_view_pos, and
// code responsible for changing view_pos
//
class SnappingGrid {
public:
	SnappingGrid();

	void draw(const int2 &view_pos, const int2 &view_size, const int2 &tile_map_size) const;
	void update();

	void setHeight(int new_height) { m_height = new_height; }
	int height() const { return m_height; }
	int cellSize() const { return m_cell_size; }
	bool isVisible() const { return m_is_visible; }

private:
	int m_height, m_cell_size;
	bool m_is_visible;
};



#endif
