/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/view.h"
#include "game/tile_map.h"
#include "editor/tile_group.h"
#include "occluder_map.h"
#include "gfx/drawing.h"

namespace ui {

	View::View(game::TileMap &tile_map, game::EntityMap &entity_map, const int2 &view_size)
		:m_tile_map(tile_map), m_entity_map(entity_map), m_occluder_config(tile_map.occluderMap()), m_height(1), m_cell_size(1),
		m_is_visible(false), m_view_size(view_size), m_view_pos(-200, 300) {
		updateVisibility();
	}

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

		DTexture::unbind();
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
							(isKeyDownAuto(InputKey::pagedown)? -1 : 0) +
							(isKeyDownAuto(InputKey::pageup)? 1 : 0);
		if(height_change)
			m_height = clamp(m_height + height_change, 0, (int)Grid::max_height);
		
		{
			int actions[TileGroup::Group::side_count] = {
				InputKey::kp_1, 
				InputKey::kp_2,
				InputKey::kp_3,
				InputKey::kp_6,
				InputKey::kp_9,
				InputKey::kp_8,
				InputKey::kp_7,
				InputKey::kp_4
			};
			
			for(int n = 0; n < arraySize(actions); n++)
				if(isKeyDownAuto(actions[n]))
					m_view_pos += worldToScreen(TileGroup::Group::s_side_offsets[n] * m_cell_size);
		}

		if((isKeyPressed(InputKey::lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();

		IRect rect = worldToScreen(IBox(int3(0, 0, 0), asXZY(m_tile_map.dimensions(), 256)));
		m_view_pos = clamp(m_view_pos, rect.min, rect.max - m_view_size);
	}

	void View::updateVisibility(int cursor_height) {
		float max_pos = m_height + cursor_height;

		const OccluderMap &occmap = m_tile_map.occluderMap();
		m_occluder_config.update();

		for(int n = 0; n < (int)occmap.size(); n++)
			m_occluder_config.setVisible(n, occmap[n].bbox.min.y <= max_pos);

		m_tile_map.updateVisibility(m_occluder_config);
		m_entity_map.updateVisibility(m_occluder_config);
	}
		
	const IBox View::computeCursor(const int2 &start, const int2 &end, const int3 &bbox, int height, int offset) const {
		float2 height_off = worldToScreen(int3(0, height, 0));
		int3 gbox(cellSize(), 1, cellSize());

		int3 start_pos = asXZ((int2)( screenToWorld(float2(start + pos()) - height_off) + float2(0.5f, 0.5f)));
		int3 end_pos   = asXZ((int2)( screenToWorld(float2(end   + pos()) - height_off) + float2(0.5f, 0.5f)));

		start_pos.y = end_pos.y = height + offset;
		
		{
			int apos1 = start_pos.x % gbox.x;
			int apos2 = apos1 - gbox.x + bbox.x;
			start_pos.x -= apos1 < gbox.x - apos1 || bbox.x >= gbox.x? apos1 : apos2;
		}
		{
			int apos1 = start_pos.z % gbox.z;
			int apos2 = apos1 - gbox.z + bbox.z;
			start_pos.z -= apos1 < gbox.z - apos1 || bbox.z >= gbox.z? apos1 : apos2;
		}
		if(end == start)
			end_pos = start_pos;
		
		int3 dir(end_pos.x >= start_pos.x? 1 : -1, 1, end_pos.z >= start_pos.z? 1 : -1);
		int3 size(abs(end_pos.x - start_pos.x), 1, abs(end_pos.z - start_pos.z));
		size += bbox - int3(1, 1, 1);
		size.x -= size.x % bbox.x;
		size.z -= size.z % bbox.z;
		size = max(bbox, size);

		if(dir.x < 0)
			start_pos.x += bbox.x;
		if(dir.z < 0)
			start_pos.z += bbox.z;
		end_pos = start_pos + dir * size;

		if(start_pos.x > end_pos.x) swap(start_pos.x, end_pos.x);
		if(start_pos.z > end_pos.z) swap(start_pos.z, end_pos.z);
		
		int2 dims = m_tile_map.dimensions();
		start_pos = asXZY(clamp(start_pos.xz(), int2(0, 0), dims), start_pos.y);
		  end_pos = asXZY(clamp(  end_pos.xz(), int2(0, 0), dims),   end_pos.y);

		return IBox(start_pos, end_pos);

	}

}
