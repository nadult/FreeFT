/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/entities_editor.h"
#include "editor/view.h"
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

	EntitiesEditor::EntitiesEditor(game::TileMap &tile_map, game::EntityMap &entity_map, View &view, IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_view(view), m_tile_map(tile_map), m_entity_map(entity_map), m_proto(nullptr) {
		m_is_selecting = false;
		m_mode = mode_selecting;
		m_selection = IRect::empty();
		m_cursor_pos = float3(0, 0, 0);
		m_proto_angle = 0;
	}
		
	void EntitiesEditor::setProto(game::Entity *proto) {
		m_proto = proto;
	}

	void EntitiesEditor::onInput(int2 mouse_pos) {
		computeCursor(mouse_pos, mouse_pos);

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
			int dir_count = m_proto->sprite().dirCount(0);

			if(inc && dir_count)
				m_proto_angle = (m_proto_angle + inc + dir_count) % dir_count;
			m_proto->setDirAngle(constant::pi * 2.0f * (float)m_proto_angle / float(dir_count));
		}

		if(isKeyPressed(Key_del)) {
			for(int i = 0; i < (int)m_selected_ids.size(); i++)
				m_entity_map.remove(m_selected_ids[i]);
			m_selected_ids.clear();
		}

		m_view.update();
	}

	void EntitiesEditor::computeCursor(int2 start, int2 end) {
		float2 height_off = worldToScreen(int3(0, 0, 0));

		start += m_view.pos();
		  end += m_view.pos();

		Ray ray = screenRay(start);

		Flags::Type flags = Flags::all;
		if(isKeyPressed(Key_lshift))
			flags = flags & ~(Flags::wall_tile | Flags::object_tile);

		auto isect = m_tile_map.trace(ray, -1, flags | Flags::visible);
		float3 pos = isect.first == -1? (float3)asXZ(screenToWorld(start)) : ray.at(isect.second);

		m_cursor_pos = (float3)round(pos);
		m_selection = IRect(min(start, end), max(start, end));
	}

	bool EntitiesEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if(key == 0 && !isKeyPressed(Key_lctrl)) {
			computeCursor(start, current);
			m_is_selecting = !is_final;
			if(m_mode == mode_selecting && is_final && is_final != -1) {
				m_selected_ids.clear();

				m_entity_map.findAll(m_selected_ids, m_selection, Flags::all | Flags::visible);
				for(int n = 0; n < (int)m_selected_ids.size(); n++) {
					auto &object = m_entity_map[m_selected_ids[n]];
					//TODO: FIX: you can select invisible (on lower level, for example) entities
					//TODO: remove hack with grid height
					if(object.bbox.max.y < m_view.gridHeight() || !areOverlapping(m_selection, object.ptr->currentScreenRect())) {
						m_selected_ids[n--] = m_selected_ids.back();
						m_selected_ids.pop_back();
					}
				}
				std::sort(m_selected_ids.begin(), m_selected_ids.end());
				computeCursor(current, current);
			}
			else if(m_mode == mode_placing && is_final)
				m_entity_map.add(PEntity(m_proto->clone()));

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
		m_view.updateVisibility();
		SceneRenderer renderer(clippedRect(), m_view.pos());

		{
			vector<int> visible_ids;
			visible_ids.reserve(1024);
			m_tile_map.findAll(visible_ids, renderer.targetRect(), Flags::all | Flags::visible);

			for(int i = 0; i < (int)visible_ids.size(); i++) {
				auto &object = m_tile_map[visible_ids[i]];
				int3 pos(object.bbox.min);
				
				object.ptr->addToRender(renderer, pos, Color::white);
			
			}

			visible_ids.clear();
			m_entity_map.findAll(visible_ids, renderer.targetRect(), Flags::all | Flags::visible);
			for(int n = 0; n < (int)visible_ids.size(); n++) {
				auto &object = m_entity_map[visible_ids[n]];
				object.ptr->addToRender(renderer);
				
				if(m_tile_map.findAny(object.bbox) != -1 || m_entity_map.findAny(object.bbox, visible_ids[n]) != -1)
					renderer.addBox(object.bbox, Color::red);
			}

			for(int n = 0; n < (int)m_selected_ids.size(); n++) {
				auto &object = m_entity_map[m_selected_ids[n]];
				renderer.addBox(object.bbox, Color::white);
			}
		}

		if(m_proto && m_mode == mode_placing) {
			m_proto->setPos(m_cursor_pos);
			m_proto->addToRender(renderer);
			FBox bbox = m_proto->boundingBox();

			bool is_colliding = m_tile_map.findAny(bbox) != -1 || m_entity_map.findAny(bbox) != -1;
			renderer.addBox(bbox, is_colliding? Color::red : Color::white);
			if(bbox.max.y == bbox.min.y)
				bbox.max.y += 1.0f;

			int over_ground = 0;
			while(over_ground < 16 && m_tile_map.findAny(bbox - float3(0, 1 + over_ground, 0)) == -1)
				over_ground++;

			if(over_ground)
				renderer.addBox(FBox(asXZY(bbox.min.xz(), bbox.min.y - over_ground), asXZY(bbox.max.xz(), bbox.min.y)),
						Color::yellow);
		}

		renderer.render();
		if(m_mode == mode_selecting && m_is_selecting)
			drawRect(m_selection, Color::white);


		setScissorRect(clippedRect());
		setScissorTest(true);
		lookAt(-clippedRect().min + m_view.pos());
		m_view.drawGrid();

		DTexture::bind0();
		
		lookAt(-clippedRect().min);
		PFont font = Font::mgr[s_font_names[1]];

		font->drawShadowed(int2(0, clippedRect().height() - 25), Color::white, Color::black,
				"Cursor: (%.0f, %.0f, %.0f)  Grid: %d Mode: %s\n",
				m_cursor_pos.x, m_cursor_pos.y, m_cursor_pos.z, m_view.gridHeight(), s_mode_strings[m_mode]);
	}

}
