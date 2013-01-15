#include "editor/entities_editor.h"
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

	EntitiesEditor::EntitiesEditor(IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_tile_map(nullptr), m_entity_map(nullptr) {
		m_view_pos = int2(0, 0);
		m_is_selecting = false;
		m_mode = mode_selecting;
		m_cursor_height = 0;
	}

	void EntitiesEditor::setTileMap(TileMap *new_tile_map) {
		//TODO: do some cleanup within the old tile map?
		m_tile_map = new_tile_map;
	}
	
	void EntitiesEditor::setEntityMap(EntityMap *new_entity_map) {
		//TODO: do some cleanup within the old tile map?
		m_entity_map = new_entity_map;
		m_selected_ids.clear();
	}

	void EntitiesEditor::onInput(int2 mouse_pos) {
		ASSERT(m_tile_map && m_entity_map);

		m_selection = computeCursor(mouse_pos, mouse_pos);
		if(isKeyDown(Key_kp_add))
			m_cursor_height++;
		if(isKeyDown(Key_kp_subtract))
			m_cursor_height--;

		//TODO: make proper accelerators
		if(isKeyDown('S')) {
			m_mode = mode_selecting;
			sendEvent(this, Event::button_clicked, m_mode);
		}
		if(isKeyDown('P')) {
			m_mode = mode_placing;
			sendEvent(this, Event::button_clicked, m_mode);
		}

		if(isKeyPressed(Key_del)) {
			for(int i = 0; i < (int)m_selected_ids.size(); i++)
				m_tile_map->remove(m_selected_ids[i]);
			m_selected_ids.clear();
		}
	}

	IBox EntitiesEditor::computeCursor(int2 start, int2 end) const {
		float2 height_off = worldToScreen(int3(0, 0, 0));

		int3 start_pos = asXZ((int2)( screenToWorld(float2(start + m_view_pos) - height_off) + float2(0.5f, 0.5f)));
		int3 end_pos   = asXZ((int2)( screenToWorld(float2(end   + m_view_pos) - height_off) + float2(0.5f, 0.5f)));

		if(start_pos.x > end_pos.x) swap(start_pos.x, end_pos.x);
		if(start_pos.z > end_pos.z) swap(start_pos.z, end_pos.z);
		
		if(m_tile_map) {
			int2 dims = m_tile_map->dimensions();
			start_pos = asXZY(clamp(start_pos.xz(), int2(0, 0), dims), start_pos.y);
			  end_pos = asXZY(clamp(  end_pos.xz(), int2(0, 0), dims),   end_pos.y);
		}

		if(m_mode == mode_selecting)
			end_pos.y = start_pos.y;

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
			if(is_final && is_final != -1) {
				if(m_mode == mode_selecting) {
					m_selected_ids.clear();
					m_entity_map->findAll(m_selected_ids, FBox(m_selection.min, m_selection.max + int3(0, 1, 0)));
					std::sort(m_selected_ids.begin(), m_selected_ids.end());
				}
			//	else if(m_mode == mode_placing && m_new_tile) {
			//		fill(m_selection);
			//	}

				m_selection = computeCursor(current, current);
			}

			return true;
		}

		return false;
	}

	void EntitiesEditor::drawBoxHelpers(const IBox &box) const {
		DTexture::bind0();

		int3 pos = box.min, bbox = box.max - box.min;
		int3 tsize = asXZY(m_tile_map->dimensions(), 32);

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
		ASSERT(m_tile_map);

		SceneRenderer renderer(clippedRect(), m_view_pos);


		{
			vector<int> visible_ids;
			visible_ids.reserve(1024);
			m_tile_map->findAll(visible_ids, renderer.targetRect());
			IRect xz_selection(m_selection.min.xz(), m_selection.max.xz());

			for(int i = 0; i < (int)visible_ids.size(); i++) {
				auto object = (*m_tile_map)[visible_ids[i]];
				int3 pos(object.bbox.min);
				
				IBox box = IBox({0,0,0}, object.ptr->bboxSize()) + pos;
				Color col =	box.max.y < m_selection.min.y? Color::gray :
							box.max.y == m_selection.min.y? Color(200, 200, 200, 255) : Color::white;
				if(areOverlapping(IRect(box.min.xz(), box.max.xz()), xz_selection))
					col.r = col.g = 255;

				object.ptr->addToRender(renderer, pos, col);
			}
			for(int i = 0; i < (int)m_selected_ids.size(); i++)
				renderer.addBox((*m_tile_map)[m_selected_ids[i]].bbox);
		}
		renderer.render();

		setScissorRect(clippedRect());
		setScissorTest(true);
		IRect view_rect = clippedRect() - m_view_pos;
		lookAt(-view_rect.min);
		int2 wsize = view_rect.size();

	/*	if(m_new_tile && (m_mode == mode_placing || m_mode == mode_placing_random || m_mode == mode_filling) && m_new_tile) {
			int3 bbox = m_new_tile->bboxSize();
		
			for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
				for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
					int3 pos(x, m_selection.min.y, z);

					bool collides = m_tile_map->findAny(FBox(pos, pos + bbox)) != -1;
					Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

					m_new_tile->draw(int2(worldToScreen(pos)), color);
					DTexture::bind0();
					drawBBox(IBox(pos, pos + bbox));
				}
	//		m_tile_map->drawBoxHelpers(IBox(pos, pos + m_new_tile->bbox));
		}*/

	//	m_tile_map->drawBoxHelpers(m_selection);
		DTexture::bind0();

		{
			IBox under = m_selection;
			under.max.y = under.min.y;
			under.min.y = 0;

			drawBBox(under, Color(127, 127, 127, 255));
			drawBBox(m_selection);
		}

		
		lookAt(-clippedRect().min);
		PFont font = Font::mgr[s_font_names[1]];

		const char *mode_names[mode_count] = {
			"selecting entities",
			"placing entities",
		};

		font->drawShadowed(int2(0, clippedRect().height() - 25), Color::white, Color::black,
				"Cursor: (%d, %d, %d)  Mode: %s\n",
				m_selection.min.x, m_selection.min.y, m_selection.min.z, mode_names[m_mode]);
	}

	void EntitiesEditor::clampViewPos() {
		DASSERT(m_tile_map);
		int2 rsize = rect().size();
		IRect rect = worldToScreen(IBox(int3(0, 0, 0), asXZY(m_tile_map->dimensions(), 32)));
		m_view_pos = clamp(m_view_pos, rect.min, rect.max - rsize);
	}

}
