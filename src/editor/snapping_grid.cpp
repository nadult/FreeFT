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

#include "editor/snapping_grid.h"
#include "gfx/device.h"

using namespace gfx;

SnappingGrid::SnappingGrid()
	:m_height(1), m_cell_size(1), m_is_visible(false) { }

void SnappingGrid::draw(const int2 &view_pos, const int2 &view_size, const int2 &tile_map_size) const {
	if(!m_is_visible)
		return;

	int2 p[4] = {
		screenToWorld(view_pos + int2(0, 0)),
		screenToWorld(view_pos + int2(0, view_size.y)),
		screenToWorld(view_pos + int2(view_size.x, view_size.y)),
		screenToWorld(view_pos + int2(view_size.x, 0)) };
	int2 offset = screenToWorld(worldToScreen(int3(0, m_height, 0)));
	for(int n = 0; n < 4; n++)
		p[n] -= offset;

	int2 tmin = min(min(p[0], p[1]), min(p[2], p[3]));
	int2 tmax = max(max(p[0], p[1]), max(p[2], p[3]));
	IRect box(max(tmin, int2(0, 0)), min(tmax, tile_map_size));

	DTexture::bind0();
	Color color(255, 255, 255, 64);

	for(int x = box.min.x - box.min.x % m_cell_size; x <= box.max.x; x += m_cell_size)
		drawLine(int3(x, m_height, box.min.y), int3(x, m_height, box.max.y), color);
	for(int y = box.min.y - box.min.y % m_cell_size; y <= box.max.y; y += m_cell_size)
		drawLine(int3(box.min.x, m_height, y), int3(box.max.x, m_height, y), color);
}

void SnappingGrid::update() {
	if(isKeyDown('G')) {
		if(m_is_visible) {
			if(m_cell_size == 3)
				m_cell_size = 6;
			else if(m_cell_size == 6)
				m_cell_size = 9;
			else {
				m_cell_size = 1;
				m_is_visible = false;
			}
		}
		else {
			m_cell_size = 3;
			m_is_visible = true;
		}
	}
		
	int wheel_move = getMouseWheelMove();
	if(wheel_move)
		m_height = clamp(m_height + wheel_move, 0, 256); //TODO: magic number
}
