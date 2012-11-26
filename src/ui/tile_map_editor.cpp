#include "ui/tile_map_editor.h"
#include "tile_group.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"

using namespace gfx;

namespace ui {

	TileMapEditor::TileMapEditor(IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_show_grid(false), m_grid_size(1, 1), m_tile_map(0), m_new_tile(nullptr) {
		m_tile_group = nullptr;
		m_view_pos = int2(0, 0);
		m_is_selecting = false;
		m_mode = mSelecting;

		m_cursor_height = 0;
		m_grid_height = 0;
		m_dirty_percent = 0.0f;
	}

	void TileMapEditor::drawGrid(const IBox &box, int2 node_size, int y) {
		DTexture::bind0();

		//TODO: proper Drawing when y != 0
		for(int x = box.min.x - box.min.x % node_size.x; x <= box.max.x; x += node_size.x)
			drawLine(int3(x, y, box.min.z), int3(x, y, box.max.z), Color(255, 255, 255, 64));
		for(int z = box.min.z - box.min.z % node_size.y; z <= box.max.z; z += node_size.y)
			drawLine(int3(box.min.x, y, z), int3(box.max.x, y, z), Color(255, 255, 255, 64));
	}

	void TileMapEditor::setTileMap(TileMap *new_tile_map) {
		//TODO: do some cleanup within the old tile map?
		m_tile_map = new_tile_map;
	}

	void TileMapEditor::onInput(int2 mouse_pos) {
		ASSERT(m_tile_map);

		m_selection = computeCursor(mouse_pos, mouse_pos);
		if(isKeyDown(Key_kp_add))
			m_cursor_height++;
		if(isKeyDown(Key_kp_subtract))
			m_cursor_height--;

		if(isKeyDown('G')) {
			if(m_show_grid) {
				if(m_grid_size.x == 3)
					m_grid_size = int2(6, 6);
				else if(m_grid_size.x == 6)
					m_grid_size = int2(9, 9);
				else {
					m_grid_size = int2(1, 1);
					m_show_grid = false;
				}
			}
			else {
				m_grid_size = int2(3, 3);
				m_show_grid = true;
			}
		}

		//TODO: make proper accelerators
		if(isKeyDown('S')) {
			m_mode = mSelecting;
			sendEvent(this, Event::button_clicked, m_mode);
		}
		if(isKeyDown('P')) {
			m_mode = mPlacing;
			sendEvent(this, Event::button_clicked, m_mode);
		}
		if(isKeyDown('R')) {
			m_mode = mPlacingRandom;
			sendEvent(this, Event::button_clicked, m_mode);
		}
		if(isKeyDown('F')) {
			m_mode = mAutoFilling;
			sendEvent(this, Event::button_clicked, m_mode);
		}

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
				if(isKeyDown(actions[n]))
					m_view_pos += worldToScreen(TileGroup::Group::s_side_offsets[n] * m_grid_size.x);
			clampViewPos();
		}


		if(isKeyPressed(Key_del))
			m_tile_map->deleteSelected();
	}

	IBox TileMapEditor::computeCursor(int2 start, int2 end) const {
		float2 height_off = worldToScreen(int3(0, m_grid_height, 0));
		int3 gbox = asXZY(m_grid_size, 1);

		bool select_mode = m_mode == mSelecting;
		int3 bbox = m_new_tile && !select_mode? m_new_tile->m_bbox : gbox;

		int3 start_pos = asXZ((int2)( screenToWorld(float2(start + m_view_pos) - height_off) + float2(0.5f, 0.5f)));
		int3 end_pos   = asXZ((int2)( screenToWorld(float2(end   + m_view_pos) - height_off) + float2(0.5f, 0.5f)));

		start_pos.y = end_pos.y = m_cursor_height;
		
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
		
		if(m_tile_map) {
			IBox map_box = m_tile_map->boundingBox();
			start_pos = clamp(start_pos, map_box.min, map_box.max);
			end_pos = clamp(end_pos, map_box.min, map_box.max);
		}

		if(select_mode)
			end_pos.y = start_pos.y;

		return IBox(start_pos, end_pos);

	}

	void TileMapEditor::fillRandomized(int group_id, const IBox &fill_box) {
		DASSERT(m_tile_group);
		DASSERT(group_id >= 0 && group_id < m_tile_group->groupCount());

		vector<int> entries, dirty_entries;
		entries.reserve(m_tile_group->groupEntryCount(group_id));
		for(int n = 0; n < m_tile_group->entryCount(); n++)
			if(m_tile_group->entryGroup(n) == group_id)
				(m_tile_group->isEntryDirty(n)? dirty_entries : entries).push_back(n);

		if(dirty_entries.empty())
			dirty_entries = entries;
		if(entries.empty())
			entries = dirty_entries;
		DASSERT(!entries.empty() && !dirty_entries.empty());

		int3 bbox = m_new_tile->m_bbox;

		for(int x = fill_box.min.x; x < fill_box.max.x; x += bbox.x)
			for(int z = fill_box.min.z; z < fill_box.max.z; z += bbox.z) {
				const vector<int> &source = rand() % 1000 < 1000 * m_dirty_percent? dirty_entries : entries;
				int random_id = rand() % source.size();
				const Tile *tile = m_tile_group->entryTile(source[random_id]);
				try { m_tile_map->addTile(*tile, int3(x, fill_box.min.y, z)); }
				catch(...) { }
			}
	}

	void TileMapEditor::fillHoles(int main_group_id, const IBox &fill_box) {
		DASSERT(m_tile_group);
		DASSERT(main_group_id >= 0 && main_group_id < m_tile_group->groupCount());

		int3 bbox = m_new_tile->m_bbox;
		int main_surf = m_tile_group->groupSurface(main_group_id, 0);
		for(int n = 1; n < TileGroup::Group::side_count; n++)
			if(m_tile_group->groupSurface(main_group_id, n) != main_surf)
				return;

		for(int n = 0; n < m_tile_group->entryCount(); n++)
			m_tile_group->entryTile(n)->m_temp = n;

		for(int x = fill_box.min.x; x < fill_box.max.x; x += bbox.x)
			for(int z = fill_box.min.z; z < fill_box.max.z; z += bbox.z) {
				int3 pos(x, fill_box.min.y, z);
				const TileInstance *neighbours[8] = {
					m_tile_map->at(pos + int3(0, 0, bbox.z)),
					m_tile_map->at(pos + int3(bbox.x, 0, 0)),
					m_tile_map->at(pos - int3(0, 0, bbox.z)),
					m_tile_map->at(pos - int3(bbox.x, 0, 0)),

					m_tile_map->at(pos + int3( bbox.x, 0,  bbox.z)),
					m_tile_map->at(pos + int3( bbox.x, 0, -bbox.z)),
					m_tile_map->at(pos + int3(-bbox.x, 0, -bbox.z)),
					m_tile_map->at(pos + int3(-bbox.x, 0,  bbox.z)),
			   	};

				int sides[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // -1: any, -2: error
				int ngroups[8];

				int soffset[8] = { 5, 7, 1, 3,   5, 7, 1, 3 };
				int doffset[8] = { 7, 1, 3, 5,   1, 3, 5, 7 };
				bool error = false;

				for(int n = 0; n < 8; n++) {
					int entry_id = neighbours[n]? m_tile_group->findEntry(neighbours[n]->m_tile) : -1;
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

				//TODO: speed up
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
					try { m_tile_map->addTile(*tile, int3(x, fill_box.min.y, z)); }
					catch(...) { }
				}
			}
	
		fillRandomized(main_group_id, fill_box);
	}

	bool TileMapEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if((isKeyPressed(Key_lctrl) && key == 0) || key == 2) {
			m_view_pos -= getMouseMove();
			clampViewPos();
			return true;
		}
		else if(key == 0) {
			m_selection = computeCursor(start, current);
			m_is_selecting = !is_final;
			if(is_final && is_final != -1) {
				if(m_mode == mSelecting) {
					m_tile_map->select(IBox(m_selection.min, m_selection.max + int3(0, 1, 0)),
							isKeyPressed(Key_lctrl)? SelectionMode::add : SelectionMode::normal);
				}
				else if(m_mode == mPlacing && m_new_tile) {
					m_tile_map->fill(*m_new_tile, IBox(m_selection.min, m_selection.max));
				}
				else if(m_mode == mPlacingRandom && m_new_tile && m_tile_group) {
					int entry_id = m_tile_group->findEntry(m_new_tile);
					int group_id = entry_id != -1? m_tile_group->entryGroup(entry_id) : -1;
					if(group_id != -1)
						fillRandomized(group_id, m_selection);
				}
				else if(m_mode == mAutoFilling && m_tile_group && m_new_tile) {
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

	void TileMapEditor::drawBoxHelpers(const IBox &box) const {
		DTexture::bind0();

		int3 pos = box.min, bbox = box.max - box.min;
		int3 tsize = asXZY(m_tile_map->size(), TileMapNode::size_y);

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
	
	void TileMapEditor::drawContents() const {
		ASSERT(m_tile_map);

		SceneRenderer renderer(clippedRect(), m_view_pos);

		{
			IRect view = renderer.targetRect();
			IRect xz_selection(m_selection.min.xz(), m_selection.max.xz());

			for(int n = 0; n < m_tile_map->nodeCount(); n++) {
				const TileMapNode &node = (*m_tile_map)(n);
				if(!node.instanceCount())
					continue;

				int3 node_pos = m_tile_map->nodePos(n);
				IRect screen_rect = node.screenRect() + worldToScreen(node_pos);

				if(!areOverlapping(screen_rect, view))
					continue;

				for(uint i = 0; i < node.m_instances.size(); i++) {
					const TileInstance &instance = node.m_instances[i];
					const gfx::Tile *tile = instance.m_tile;
					int3 pos = instance.pos() + node_pos;
					
					gfx::PTexture tex = tile->dTexture;
					IBox box = IBox({0,0,0}, tile->m_bbox) + pos;
					Color col =	box.max.y < m_selection.min.y? Color::gray :
								box.max.y == m_selection.min.y? Color(200, 200, 200, 255) : Color::white;
					if(areOverlapping(IRect(box.min.xz(), box.max.xz()), xz_selection))
						col.r = col.g = 255;

					renderer.add(tex, IRect({0, 0}, tex->size()) - tile->m_offset, pos, tile->m_bbox, col);
					if(instance.isSelected())
						renderer.addBox(IBox(pos, pos + tile->m_bbox));
				}
			}
					
		}
		renderer.render();

		setScissorRect(clippedRect());
		setScissorTest(true);
		IRect view_rect = clippedRect() - m_view_pos;
		lookAt(-view_rect.min);
		int2 wsize = view_rect.size();

		if(m_show_grid) {
			int2 p[4] = {
				screenToWorld(m_view_pos + int2(0, 0)),
				screenToWorld(m_view_pos + int2(0, wsize.y)),
				screenToWorld(m_view_pos + int2(wsize.x, wsize.y)),
				screenToWorld(m_view_pos + int2(wsize.x, 0)) };


			int2 tmin = min(min(p[0], p[1]), min(p[2], p[3]));
			int2 tmax = max(max(p[0], p[1]), max(p[2], p[3]));
			IBox box(tmin.x, 0, tmin.y, tmax.x, 0, tmax.y);
			IBox bbox = m_tile_map->boundingBox();
			box = IBox(max(box.min, bbox.min), min(box.max, bbox.max));

			drawGrid(box, m_grid_size, m_grid_height);
		}
		
		if(m_new_tile && (m_mode == mPlacing || m_mode == mPlacingRandom || m_mode == mAutoFilling) && m_new_tile) {
			int3 bbox = m_new_tile->m_bbox;
		
			for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
				for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
					int3 pos(x, m_selection.min.y, z);

					bool collides = m_tile_map->isOverlapping(IBox(pos, pos + bbox));
					Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

					m_new_tile->draw(int2(worldToScreen(pos)), color);
					DTexture::bind0();
					drawBBox(IBox(pos, pos + bbox));
				}
	//		m_tile_map->drawBoxHelpers(IBox(pos, pos + m_new_tile->bbox));
		}

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

		const char *mode_names[mCount] = {
			"selecting tiles",
			"placing new tiles",
			"placing new tiles (randomized)",
			"filling holes",
		};

		font->drawShadowed(int2(0, clippedRect().height() - 25), Color::white, Color::black,
				"Cursor: (%d, %d, %d)  Mode: %s\n",
				m_selection.min.x, m_selection.min.y, m_selection.min.z, mode_names[m_mode]);
	}

	void TileMapEditor::clampViewPos() {
		DASSERT(m_tile_map);
		int2 rsize = rect().size();
		IRect rect = worldToScreen(m_tile_map->boundingBox());
		m_view_pos = clamp(m_view_pos, rect.min, rect.max - rsize);
	}

}
