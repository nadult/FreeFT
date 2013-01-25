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

#include "editor/view.h"
#include "game/tile_map.h"
#include "editor/tile_group.h"
#include "occluder_map.h"
#include "gfx/device.h"

using namespace gfx;

namespace ui {

	View::View(game::TileMap &tile_map, const int2 &view_size)
		:m_tile_map(tile_map), m_height(1), m_cell_size(1), m_is_visible(false), m_view_size(view_size), m_view_pos(-200, 300) { }

	void View::drawGrid() const {
		if(!m_is_visible)
			return;

		const int2 tile_map_size = m_tile_map.dimensions();


		int2 p[4] = {
			screenToWorld(m_view_pos + int2(0, 0)),
			screenToWorld(m_view_pos + int2(0, m_view_size.y)),
			screenToWorld(m_view_pos + int2(m_view_size.x, m_view_size.y)),
			screenToWorld(m_view_pos + int2(m_view_size.x, 0)) };
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

	void View::update() {
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
			
		int height_change = getMouseWheelMove() +
							(isKeyDownAuto(Key_pagedown)? -1 : 0) +
							(isKeyDownAuto(Key_pageup)? 1 : 0);
		if(height_change)
			m_height = clamp(m_height + height_change, 0, (int)Grid::max_height);
		
		{
			KeyId actions[TileGroup::Group::side_count] = {
				Key_kp_1, 
				Key_kp_2,
				Key_kp_3,
				Key_kp_6,
				Key_kp_9,
				Key_kp_8,
				Key_kp_7,
				Key_kp_4
			};
			
			for(int n = 0; n < COUNTOF(actions); n++)
				if(isKeyDownAuto(actions[n]))
					m_view_pos += worldToScreen(TileGroup::Group::s_side_offsets[n] * m_cell_size);
		}

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();

		IRect rect = worldToScreen(IBox(int3(0, 0, 0), asXZY(m_tile_map.dimensions(), 256)));
		m_view_pos = clamp(m_view_pos, rect.min, rect.max - m_view_size);
	}

	void View::updateVisibility(int cursor_height) {
		OccluderMap &occmap = m_tile_map.occluderMap();
		float max_pos = m_height + cursor_height;
		bool has_changed = false;

		for(int n = 0; n < occmap.size(); n++) {
			bool is_visible =occmap[n].bbox.min.y <= max_pos;
			has_changed |= is_visible != occmap[n].is_visible;
			occmap[n].is_visible = is_visible;
		}

		if(has_changed) {
			m_tile_map.updateVisibility();
		}
	}

}
