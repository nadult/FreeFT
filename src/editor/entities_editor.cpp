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

#include "editor/entities_editor.h"
#include "editor/snapping_grid.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"
#include "game/tile_map.h"
#include "game/entity_map.h"
#include "game/tile.h"
#include <algorithm>
#include <cstdlib>

using namespace gfx;
using namespace game;

namespace ui {

	static const char *s_mode_strings[EntitiesEditor::mode_count] = {
		"[S]electing entities",
		"[P]lacing entities",
	};
	const char **EntitiesEditor::modeStrings() { return s_mode_strings; }

	EntitiesEditor::EntitiesEditor(game::TileMap &tile_map, game::EntityMap &entity_map, SnappingGrid &grid, IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_grid(grid), m_tile_map(tile_map), m_entity_map(entity_map), m_proto(nullptr) {
		m_view_pos = int2(0, 0);
		m_is_selecting = false;
		m_mode = mode_selecting;
		m_selection = IBox::empty();
		m_proto_angle = 0;
	}

	void EntitiesEditor::onInput(int2 mouse_pos) {
		m_selection = computeCursor(mouse_pos, mouse_pos);

		//TODO: make proper accelerators
		if(isKeyDown('S')) {
			m_mode = mode_selecting;
			sendEvent(this, Event::button_clicked, m_mode);
		}
		if(isKeyDown('P')) {
			m_mode = mode_placing;
			sendEvent(this, Event::button_clicked, m_mode);
		}

		if(m_proto) {
			int inc = 0;
			if(isKeyDown(Key_left)) inc = -1;
			if(isKeyDown(Key_right)) inc = 1;

			if(inc) {
				float2 old_dir = m_proto->actualDir();

				do {
					m_proto_angle = (m_proto_angle + inc + 16) % 16;
					m_proto->setDirAngle(constant::pi * 2.0f * (float)m_proto_angle / 16.0f);
				} while(m_proto->actualDir() == old_dir);
			}
		}

		if(isKeyPressed(Key_del)) {
			for(int i = 0; i < (int)m_selected_ids.size(); i++)
				m_entity_map.remove(m_selected_ids[i]);
			m_selected_ids.clear();
		}

		m_grid.update();
	}

	IBox EntitiesEditor::computeCursor(int2 start, int2 end) const {
		float2 height_off = worldToScreen(int3(0, 0, 0));

		start += m_view_pos;
		  end += m_view_pos;

		Ray start_ray = screenRay(start);
		Ray   end_ray = screenRay(end);

		int flags = collider_all;
		if(isKeyPressed(Key_lshift))
			flags &= ~(collider_tile_walls | collider_tile_objects);

		auto start_isect = m_tile_map.trace(start_ray, -1, flags|visibility_flag);
		auto   end_isect = m_tile_map.trace(  end_ray, -1, flags|visibility_flag);

		int3 start_pos = start_isect.first == -1? asXZ(screenToWorld(start)) : (int3)start_ray.at(start_isect.second);
		int3   end_pos =   end_isect.first == -1? asXZ(screenToWorld(  end)) : (int3)  end_ray.at(start_isect.second);

		if(end_pos == start_pos)
			end_pos = start_pos + int3(1, 0, 1);

		return IBox(start_pos, end_pos);
	}

	bool EntitiesEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {

		if((isKeyPressed(Key_lctrl) && key == 0) || key == 2) {
			m_view_pos -= getMouseMove();
			clampViewPos();
			return true;
		}
		else if(key == 0) {
			m_selection = computeCursor(start, current);
			m_is_selecting = !is_final;
			if(m_mode == mode_selecting && is_final && is_final != -1) {
				m_selected_ids.clear();
				m_entity_map.findAll(m_selected_ids, FBox(m_selection.min, m_selection.max + int3(0, 1, 0)));
				std::sort(m_selected_ids.begin(), m_selected_ids.end());
				m_selection = computeCursor(current, current);
			}
			else if(m_mode == mode_placing && is_final)
				m_entity_map.add(m_proto->clone());

			return true;
		}

		return false;
	}

	void EntitiesEditor::drawBoxHelpers(const IBox &box) const {
		DTexture::bind0();

		int3 pos = box.min, bbox = box.max - box.min;
		int3 tsize = asXZY(m_tile_map.dimensions(), 32);

		drawLine(int3(0, pos.y, pos.z), int3(tsize.x, pos.y, pos.z), Color(0, 255, 0, 127));
		drawLine(int3(0, pos.y, pos.z + bbox.z), int3(tsize.x, pos.y, pos.z + bbox.z), Color(0, 255, 0, 127));
		
		drawLine(int3(pos.x, pos.y, 0), int3(pos.x, pos.y, tsize.z), Color(0, 255, 0, 127));
		drawLine(int3(pos.x + bbox.x, pos.y, 0), int3(pos.x + bbox.x, pos.y, tsize.z), Color(0, 255, 0, 127));

		int3 tpos(pos.x, 0, pos.z);
		drawBBox(IBox(tpos, tpos + int3(bbox.x, pos.y, bbox.z)), Color(0, 0, 255, 127));
		
		drawLine(int3(0, 0, pos.z), int3(tsize.x, 0, pos.z), Color(0, 0, 255, 127));
		drawLine(int3(0, 0, pos.z + bbox.z), int3(tsize.x, 0, pos.z + bbox.z), Color(0, 0, 255, 127));
		
		drawLine(int3(pos.x, 0, 0), int3(pos.x, 0, tsize.z), Color(0, 0, 255, 127));
		drawLine(int3(pos.x + bbox.x, 0, 0), int3(pos.x + bbox.x, 0, tsize.z), Color(0, 0, 255, 127));
	}
	
	void EntitiesEditor::drawContents() const {
		SceneRenderer renderer(clippedRect(), m_view_pos);

		{			
			OccluderMap &occmap = m_tile_map.occluderMap();
			float max_pos = m_grid.height();
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


		{
			vector<int> visible_ids;
			visible_ids.reserve(1024);
			m_tile_map.findAll(visible_ids, renderer.targetRect(), collider_all|visibility_flag);

			for(int i = 0; i < (int)visible_ids.size(); i++) {
				auto &object = m_tile_map[visible_ids[i]];
				int3 pos(object.bbox.min);
				
				IBox box = IBox({0,0,0}, object.ptr->bboxSize()) + pos;
				object.ptr->addToRender(renderer, pos, Color::white);
			}

			visible_ids.clear();
			m_entity_map.findAll(visible_ids, renderer.targetRect(), collider_all|visibility_flag);
			for(int n = 0; n < (int)visible_ids.size(); n++) {
				auto &object = m_entity_map[visible_ids[n]];
				object.ptr->addToRender(renderer);
			}

			for(int n = 0; n < (int)m_selected_ids.size(); n++) {
				auto &object = m_entity_map[m_selected_ids[n]];
				renderer.addBox(object.bbox, Color::white);
			}
		}

		if(m_proto && m_mode == mode_placing) {
			m_proto->setPos(m_selection.min);
			m_proto->addToRender(renderer);
			FBox bbox = m_proto->boundingBox();

			bool is_colliding = m_tile_map.findAny(bbox) != -1;
			renderer.addBox(bbox, is_colliding? Color::red : Color::white);

			int over_ground = 0;
			while(over_ground < 16 && m_tile_map.findAny(bbox - float3(0, 1 + over_ground, 0)) == -1)
				over_ground++;

			if(over_ground)
				renderer.addBox(FBox(asXZY(bbox.min.xz(), bbox.min.y - over_ground), asXZY(bbox.max.xz(), bbox.min.y)),
						Color::yellow);
		}
		else {
			renderer.addBox(m_selection, Color::white);
		}

		renderer.render();


		setScissorRect(clippedRect());
		setScissorTest(true);
		IRect view_rect = clippedRect() - m_view_pos;
		lookAt(-view_rect.min);
		
		m_grid.draw(m_view_pos, view_rect.size(), m_tile_map.dimensions());

	//	m_tile_map.drawBoxHelpers(m_selection);
		DTexture::bind0();
		
		lookAt(-clippedRect().min);
		PFont font = Font::mgr[s_font_names[1]];

		font->drawShadowed(int2(0, clippedRect().height() - 25), Color::white, Color::black,
				"Cursor: (%d, %d, %d)  Grid: %d Mode: %s\n",
				m_selection.min.x, m_selection.min.y, m_selection.min.z, m_grid.height(), s_mode_strings[m_mode]);
	}

	void EntitiesEditor::clampViewPos() {
		int2 rsize = rect().size();
		IRect rect = worldToScreen(IBox(int3(0, 0, 0), asXZY(m_tile_map.dimensions(), 256)));
		m_view_pos = clamp(m_view_pos, rect.min, rect.max - rsize);
	}

}
