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

	void View::drawGrid(Renderer2D &out) const {
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

		int2 tmin = vmin(vmin(p[0], p[1]), vmin(p[2], p[3]));
		int2 tmax = vmax(vmax(p[0], p[1]), vmax(p[2], p[3]));
		IRect box(vmax(tmin, int2(0, 0)), vmin(tmax, tile_map_size));

		Color color(255, 255, 255, 64);
		for(int x = box.x() - box.x() % m_cell_size; x <= box.ex(); x += m_cell_size)
			drawLine(out, int3(x, m_height, box.y()), int3(x, m_height, box.ey()), color);
		for(int y = box.y() - box.y() % m_cell_size; y <= box.ey(); y += m_cell_size)
			drawLine(out, int3(box.x(), m_height, y), int3(box.ex(), m_height, y), color);
	}

	void View::update(const InputState &state) {
		if(state.isKeyDown('g')) {
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
			
		int height_change = state.mouseWheelMove() +
							(state.isKeyDownAuto(InputKey::pagedown)? -1 : 0) +
							(state.isKeyDownAuto(InputKey::pageup)? 1 : 0);
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
				if(state.isKeyDownAuto(actions[n]))
					m_view_pos += worldToScreen(TileGroup::Group::s_side_offsets[n] * m_cell_size);
		}

		if((state.isKeyPressed(InputKey::lctrl) && state.isMouseButtonPressed(InputButton::left)) ||
		   state.isMouseButtonPressed(InputButton::middle))
			m_view_pos -= state.mouseMove();

		IRect rect = worldToScreen(IBox(int3(0, 0, 0), asXZY(m_tile_map.dimensions(), 256)));
		m_view_pos = vclamp(m_view_pos, rect.min(), rect.max() - m_view_size);
	}

	void View::updateVisibility(int cursor_height) {
		float max_pos = m_height + cursor_height;

		const OccluderMap &occmap = m_tile_map.occluderMap();
		m_occluder_config.update();

		for(int n = 0; n < (int)occmap.size(); n++)
			m_occluder_config.setVisible(n, occmap[n].bbox.y() <= max_pos);

		m_tile_map.updateVisibility(m_occluder_config);
		m_entity_map.updateVisibility(m_occluder_config);
	}
		
	const IBox View::computeCursor(const int2 &start, const int2 &end, const int3 &bbox, int height, int offset) const {
		float2 height_off = worldToScreen(float3(0, height, 0));
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
		int3 size(::abs(end_pos.x - start_pos.x), 1, ::abs(end_pos.z - start_pos.z));
		size += bbox - int3(1, 1, 1);
		size.x -= size.x % bbox.x;
		size.z -= size.z % bbox.z;
		size = vmax(bbox, size);

		if(dir.x < 0)
			start_pos.x += bbox.x;
		if(dir.z < 0)
			start_pos.z += bbox.z;
		end_pos = start_pos + dir * size;

		if(start_pos.x > end_pos.x) swap(start_pos.x, end_pos.x);
		if(start_pos.z > end_pos.z) swap(start_pos.z, end_pos.z);
		
		int2 dims = m_tile_map.dimensions();
		start_pos = asXZY(vclamp(start_pos.xz(), int2(0, 0), dims), start_pos.y);
		  end_pos = asXZY(vclamp(  end_pos.xz(), int2(0, 0), dims),   end_pos.y);

		return IBox(start_pos, end_pos);

	}

}
