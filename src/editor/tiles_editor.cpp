/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/tiles_editor.h"
#include "editor/tile_group.h"
#include "editor/view.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"
#include "game/tile.h"
#include "game/tile_map.h"
#include <algorithm>
#include <cstdlib>

using namespace gfx;
using namespace game;

namespace ui {

	static const char *s_mode_strings[TilesEditor::mode_count] = {
		"[S]electing (normal)",
		"[S]electing (union)",
		"[S]electing (intersection)",
		"[S]electing (difference)",

		"[P]lacing",
		"[P]lacing (removing obstacles)",
		
		"Placing [r]andom",
		"Placing [r]andom (removing obstacles)",

		"[F]illing holes",

		"Creating/removing [o]ccluders",
	};

	const char **TilesEditor::modeStrings() { return s_mode_strings; }

	TilesEditor::TilesEditor(TileMap &tile_map, View &view, IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_view(view), m_tile_map(tile_map), m_new_tile(nullptr) {
		m_tile_group = nullptr;
		m_is_selecting = false;
		m_mode = mode_selecting_normal;

		m_cursor_offset = 0;
		m_dirty_percent = 0.0f;

		m_current_occluder = -1;
		m_mouseover_tile_id = -1;

		m_selection = IBox::empty();
	}

	void TilesEditor::setMode(Mode mode) {
		DASSERT(mode >= 0 && mode < mode_count);
		m_mode = mode;
		sendEvent(this, Event::button_clicked, m_mode);
	}

	void TilesEditor::onInput(int2 mouse_pos) {
		m_selection = computeCursor(mouse_pos, mouse_pos);

		if(isKeyDown(Key_kp_add))
			m_cursor_offset++;
		if(isKeyDown(Key_kp_subtract))
			m_cursor_offset--;

		m_view.update();

		if(isKeyDown('S'))
			setMode(isSelecting() && m_mode != mode_selecting_difference? (Mode)(m_mode + 1) : mode_selecting_normal);
		if(isKeyDown('P'))
			setMode(m_mode == mode_placing? mode_replacing : mode_placing);
		if(isKeyDown('R'))
			setMode(m_mode == mode_placing_random? mode_replacing_random : mode_placing_random);
		if(isKeyDown('F'))
			setMode(mode_filling);
		if(isKeyDown('O'))
			setMode(mode_occluders);

		OccluderMap &occmap = m_tile_map.occluderMap();
		int2 screen_pos = mouse_pos + m_view.pos();
		Ray screen_ray = screenRay(screen_pos);
		
		m_mouseover_tile_id = m_tile_map.pixelIntersect(screen_pos, Flags::all | Flags::visible);
		if(m_mouseover_tile_id == -1)
			m_mouseover_tile_id = m_tile_map.trace(screen_ray).first;
			
		m_current_occluder = m_mouseover_tile_id == -1? -1 : m_tile_map[m_mouseover_tile_id].occluder_id;

		if(isChangingOccluders()) {
			if(isMouseKeyDown(0)) {
				if(m_current_occluder == -1 && m_mouseover_tile_id != -1)
					m_current_occluder = occmap.addOccluder(m_mouseover_tile_id, m_view.gridHeight());
				else {
					occmap.removeOccluder(m_current_occluder);
					m_current_occluder = -1;
					m_mouseover_tile_id = -1;
				}
			}
		}
		else {
			if(isKeyPressed(Key_del)) {
				for(int i = 0; i < (int)m_selected_ids.size(); i++)
					m_tile_map.remove(m_selected_ids[i]);
				m_selected_ids.clear();
			}
		}
	}

	void TilesEditor::removeAll(const IBox &box) {
		vector<int> colliders;
		m_tile_map.findAll(colliders, (FBox)box);
		sort(colliders.begin(), colliders.end());

		vector<int> new_selection(m_selected_ids.size());
		vector<int>::iterator end_it = std::set_difference(m_selected_ids.begin(), m_selected_ids.end(),
				colliders.begin(), colliders.end(), new_selection.begin());
		new_selection.resize(end_it - new_selection.begin());
		m_selected_ids.swap(new_selection);

		for(int i = 0; i < (int)colliders.size(); i++)
			m_tile_map.remove(colliders[i]);
	}

	void TilesEditor::fill(const IBox &fill_box, bool is_randomized, int group_id) {
		int3 bbox = m_new_tile->bboxSize();
		
		if(isReplacing()) {
			IBox col_box = fill_box;
			col_box.max = col_box.min +
				int3(col_box.width() / bbox.x, col_box.height() / bbox.y, col_box.depth() / bbox.z) * bbox;
			removeAll(col_box);
		}
		
		vector<int> entries, dirty_entries;
		if(is_randomized) {
			DASSERT(m_tile_group);
			DASSERT(group_id >= 0 && group_id < m_tile_group->groupCount());

			entries.reserve(m_tile_group->groupEntryCount(group_id));
			for(int n = 0; n < m_tile_group->entryCount(); n++)
				if(m_tile_group->entryGroup(n) == group_id)
					(m_tile_group->isEntryDirty(n)? dirty_entries : entries).push_back(n);

			if(dirty_entries.empty())
				dirty_entries = entries;
			if(entries.empty())
				entries = dirty_entries;
			DASSERT(!entries.empty() && !dirty_entries.empty());
		}

		for(int x = fill_box.min.x; x < fill_box.max.x; x += bbox.x)
			for(int z = fill_box.min.z; z < fill_box.max.z; z += bbox.z) {
				const Tile *tile = m_new_tile;

				if(is_randomized) {
					const vector<int> &source = rand() % 1000 < 1000 * m_dirty_percent? dirty_entries : entries;
					int random_id = rand() % source.size();
					tile = m_tile_group->entryTile(source[random_id]);
				}

				try { m_tile_map.add(tile, int3(x, fill_box.min.y, z)); }
				catch(...) { }
			}
	}

	int TilesEditor::findAt(const int3 &pos) const {
		return m_tile_map.findAny(FBox(pos, pos + int3(1, 1, 1)));
	}

	void TilesEditor::fillHoles(int main_group_id, const IBox &fill_box) {
		DASSERT(m_tile_group);
		DASSERT(main_group_id >= 0 && main_group_id < m_tile_group->groupCount());

		int3 bbox = m_new_tile->bboxSize();
		int main_surf = m_tile_group->groupSurface(main_group_id, 0);
		for(int n = 1; n < TileGroup::Group::side_count; n++)
			if(m_tile_group->groupSurface(main_group_id, n) != main_surf)
				return;

		for(int n = 0; n < m_tile_group->entryCount(); n++)
			m_tile_group->entryTile(n)->m_temp = n;

		for(int x = fill_box.min.x; x < fill_box.max.x; x += bbox.x)
			for(int z = fill_box.min.z; z < fill_box.max.z; z += bbox.z) {
				int3 pos(x, fill_box.min.y, z);
				int neighbours[8] = {
					findAt(pos + int3(0, 0, bbox.z)),
					findAt(pos + int3(bbox.x, 0, 0)),
					findAt(pos - int3(0, 0, bbox.z)),
					findAt(pos - int3(bbox.x, 0, 0)),

					findAt(pos + int3( bbox.x, 0,  bbox.z)),
					findAt(pos + int3( bbox.x, 0, -bbox.z)),
					findAt(pos + int3(-bbox.x, 0, -bbox.z)),
					findAt(pos + int3(-bbox.x, 0,  bbox.z)),
			   	};

				int sides[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // -1: any, -2: error
				int ngroups[8];

				int soffset[8] = { 5, 7, 1, 3,   5, 7, 1, 3 };
				int doffset[8] = { 7, 1, 3, 5,   1, 3, 5, 7 };
				bool error = false;

				for(int n = 0; n < 8; n++) {
					const Tile *ntile = neighbours[n] == -1? nullptr : m_tile_map[neighbours[n]].ptr;

					int entry_id = ntile? m_tile_group->findEntry(ntile) : -1;
					ngroups[n] = entry_id == -1? -1 : m_tile_group->entryGroup(entry_id);
					if(ngroups[n] == -1)
						continue;

					if(n < 4) {
						for(int s = 0; s < 3; s++) {
							int src_surf = m_tile_group->groupSurface(ngroups[n], (soffset[n] - s + 8) % 8);
							int dst_idx = (doffset[n] + s) % 8;
							if(sides[dst_idx] != -1 && sides[dst_idx] != src_surf)
								error = true;
							sides[dst_idx] = src_surf;
						}
					}
					else {
						int src_surf = m_tile_group->groupSurface(ngroups[n], soffset[n]);
						int dst_idx = doffset[n];
						if(sides[dst_idx] != -1 && sides[dst_idx] != src_surf)
							error = true;
						sides[dst_idx] = src_surf;
					}
				}
				for(int n = 0; n < COUNTOF(sides); n++) {
					int prev = sides[(n + 7) % 8];
					int next = sides[(n + 1) % 8];
					if(sides[n] == -1 && (next == -1 || next == main_surf) && (prev == -1 || prev == main_surf))
						sides[n] = main_surf;
				}

				bool all_sides_main = true;
				for(int n = 0; n < COUNTOF(sides); n++)
					if(sides[n] != main_surf)
						all_sides_main = false;

				if(all_sides_main || error)
					continue;

				vector<int> entries;
				for(int n = 0; n < m_tile_group->entryCount(); n++) {
					int group_id = m_tile_group->entryGroup(n);
					const int *group_surf = m_tile_group->groupSurface(group_id);
					bool error = false;

					for(int s = 0; s < 8; s++)
						if(sides[s] != group_surf[s] && sides[s] != -1) {
							error = true;
							break;
						}
					if(!error)
						entries.push_back(n);
				}

				if(!entries.empty()) {
					int random_id = rand() % entries.size();
					const Tile *tile = m_tile_group->entryTile(entries[random_id]);
					try { m_tile_map.add(tile, int3(x, fill_box.min.y, z)); }
					catch(...) { }
				}
			}
	
		fill(fill_box, true, main_group_id);
	}

	bool TilesEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if(key == 0 && !isKeyPressed(Key_lctrl) && !isChangingOccluders()) {
			m_selection = computeCursor(start, current);
			m_is_selecting = !is_final;
			if(is_final && is_final != -1) {
				if(isSelecting()) {
					vector<int> new_ids;
					m_tile_map.findAll(new_ids, FBox(m_selection.min, m_selection.max + int3(0, 1, 0)));
					std::sort(new_ids.begin(), new_ids.end());

					if(m_mode == mode_selecting_normal)
						m_selected_ids = new_ids;
					else {
						vector<int> out;
						out.resize(new_ids.size() + m_selected_ids.size());

						vector<int>::iterator end_it;
						if(m_mode == mode_selecting_union)
							end_it = set_union(m_selected_ids.begin(), m_selected_ids.end(),
									new_ids.begin(), new_ids.end(), out.begin());
						else if(m_mode == mode_selecting_intersection)
							end_it = set_intersection(m_selected_ids.begin(), m_selected_ids.end(),
									new_ids.begin(), new_ids.end(), out.begin());
						else if(m_mode == mode_selecting_difference)
							end_it = set_difference(m_selected_ids.begin(), m_selected_ids.end(),
									new_ids.begin(), new_ids.end(), out.begin());
						out.resize(end_it - out.begin());
						m_selected_ids = out;
					}
				}
				else if(isPlacing() && m_new_tile) {
					fill(m_selection);
				}
				else if(isPlacingRandom() && m_new_tile && m_tile_group) {
					int entry_id = m_tile_group->findEntry(m_new_tile);
					int group_id = entry_id != -1? m_tile_group->entryGroup(entry_id) : -1;
					if(group_id != -1)
						fill(m_selection, true, group_id);
				}
				else if(isFilling() && m_tile_group && m_new_tile) {
					int entry_id = m_tile_group->findEntry(m_new_tile);
					int group_id = entry_id != -1? m_tile_group->entryGroup(entry_id) : -1;
					if(group_id != -1)
						fillHoles(group_id, m_selection);
				}
			
				m_selection = computeCursor(current, current);
			}

			return true;
		}

		return false;
	}

	void TilesEditor::drawBoxHelpers(const IBox &box) const {
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
	
	void TilesEditor::drawContents() const {
		m_view.updateVisibility(m_cursor_offset);
		SceneRenderer renderer(clippedRect(), m_view.pos());

		{
			vector<int> visible_ids;
			visible_ids.reserve(1024 * 8);
			m_tile_map.findAll(visible_ids, renderer.targetRect(), Flags::all | Flags::visible);

			IRect xz_selection(m_selection.min.xz(), m_selection.max.xz());
			vector<Color> tile_colors(visible_ids.size(), Color::white);

			if(isChangingOccluders()) {
				OccluderMap &occmap = m_tile_map.occluderMap();

				int new_occluder = -1;
				if(m_current_occluder == -1 && m_mouseover_tile_id != -1)
					new_occluder = occmap.addOccluder(m_mouseover_tile_id, m_view.gridHeight());

				Color colors[] = {
					Color(255, 200, 200),
					Color(255, 255, 200),
					Color(255, 200, 255),
					Color(200, 255, 255),
					Color(200, 255, 200),
					Color(200, 200, 255)
				};

				int selected_occluder = new_occluder == -1? m_current_occluder : new_occluder;

				if(selected_occluder != -1) {
					PodArray<int> is_occluder_selected(occmap.size());
					for(int n = 0; n < occmap.size(); n++)
						is_occluder_selected[n] = n == selected_occluder? 1 : occmap.isUnder(selected_occluder, n);

					for(int n = 0; n < (int)visible_ids.size(); n++) {
						auto object = m_tile_map[visible_ids[n]];
						int3 pos(object.bbox.min);
				
						Color col = Color::white;
						if(object.occluder_id != -1) {
							col = colors[object.occluder_id % COUNTOF(colors)];
							if(is_occluder_selected[object.occluder_id])
								col.a = 127;
						}
						tile_colors[n] = col;
					}
				}
				
				if(new_occluder != -1)
					occmap.removeOccluder(new_occluder);
			}
			else {
				for(int n = 0; n < (int)visible_ids.size(); n++) {
					auto object = m_tile_map[visible_ids[n]];
					int3 pos(object.bbox.min);

					IBox box = IBox({0,0,0}, object.ptr->bboxSize()) + pos;
					Color col =	box.max.y < m_selection.min.y? Color::gray :
								box.max.y == m_selection.min.y? Color(200, 200, 200, 255) : Color::white;
					if(areOverlapping(IRect(box.min.xz(), box.max.xz()), xz_selection))
						col.r = col.g = 255;
					tile_colors[n] = col;
				}
			}

			for(int i = 0; i < (int)visible_ids.size(); i++) {
				auto object = m_tile_map[visible_ids[i]];
				int3 pos(object.bbox.min);
				
//				if(m_tile_map.findAny(object.bbox, visible_ids[i]) != -1)
//					renderer.addBox(object.bbox, Color::red, false);
				object.ptr->addToRender(renderer, pos, tile_colors[i]);
			}
			for(int i = 0; i < (int)m_selected_ids.size(); i++)
				renderer.addBox(m_tile_map[m_selected_ids[i]].bbox);

		}
		renderer.render();

		setScissorRect(clippedRect());
		setScissorTest(true);
		lookAt(-clippedRect().min + m_view.pos());
		m_view.drawGrid();
		
		if(m_new_tile && (isPlacing() || isPlacingRandom() || isFilling()) && m_new_tile) {
			int3 bbox = m_new_tile->bboxSize();
		
			for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
				for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
					int3 pos(x, m_selection.min.y, z);

					bool collides = m_tile_map.findAny(FBox(pos, pos + bbox)) != -1;
					Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

					m_new_tile->draw(int2(worldToScreen(pos)), color);
					DTexture::bind0();
					drawBBox(IBox(pos, pos + bbox));
				}
	//		m_tile_map.drawBoxHelpers(IBox(pos, pos + m_new_tile->bbox));
		}

	//	m_tile_map.drawBoxHelpers(m_selection);
		DTexture::bind0();

		if(!isChangingOccluders()) {
			IBox under = m_selection;
			under.max.y = under.min.y;
			under.min.y = m_view.gridHeight();

			drawBBox(under, Color(127, 127, 127, 255));
			drawBBox(m_selection);
		}
		
		lookAt(-clippedRect().min);
		PFont font = Font::mgr[s_font_names[1]];

		font->drawShadowed(int2(0, 0), Color::white, Color::black, "Tile count: %d\n", m_tile_map.size());
		if(isChangingOccluders() && m_current_occluder != -1) {
			auto &occluder = m_tile_map.occluderMap()[m_current_occluder];
			font->drawShadowed(int2(0, 25), Color::white, Color::black, "Occluder: %d (%d objects)\n",
				   	m_current_occluder, (int)occluder.objects.size());
		}

		if(m_new_tile)
			font->drawShadowed(int2(0, clippedRect().height() - 50), Color::white, Color::black,
					"Tile: %s\n", m_new_tile->resourceName());
		font->drawShadowed(int2(0, clippedRect().height() - 25), Color::white, Color::black,
				"Cursor: (%d, %d, %d)  Grid: %d Mode: %s\n",
				m_selection.min.x, m_selection.min.y, m_selection.min.z, m_view.gridHeight(), s_mode_strings[m_mode]);
	}
		
	const IBox TilesEditor::computeCursor(const int2 &start, const int2 &end) const {
		int3 bbox(m_view.cellSize(), 1, m_view.cellSize());
		
		if(m_new_tile && !isSelecting())
			bbox = m_new_tile->bboxSize();
		IBox out = m_view.computeCursor(start, end, bbox, m_view.gridHeight(), m_cursor_offset);

		if(isSelecting())
			out.max.y = out.min.y;

		return out;
	}

}
