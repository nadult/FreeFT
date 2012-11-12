#include "tile_map_editor.h"
#include "tile_group.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"

using namespace gfx;


TileMapEditor::TileMapEditor(IRect rect)
	:ui::Window(rect, Color(0, 0, 0)), m_show_grid(false), m_grid_size(1, 1), m_tile_map(0), m_new_tile(nullptr) {
	m_tile_group = nullptr;
	m_view_pos = int2(0, 0);
	m_is_selecting = false;
	m_mode = mSelecting;

	m_cursor_height = 0;
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

	if(isKeyDown('S'))
		m_mode = mSelecting;
	if(isKeyDown('P'))
		m_mode = mPlacing;
	if(isKeyDown('R'))
		m_mode = mPlacingRandom;
	if(isKeyDown('F'))
		m_mode = mAutoFilling;

	{
		KeyId actions[TileGroup::Group::sideCount] = {
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
	}


	if(isKeyPressed(Key_del))
		m_tile_map->deleteSelected();
}

IBox TileMapEditor::computeCursor(int2 start, int2 end) const {
	float2 height_off = worldToScreen(int3(0, m_cursor_height, 0));
	int3 gbox = asXZY(m_grid_size, 1);

	bool select_mode = m_mode == mSelecting || m_mode == mAutoFilling;
	int3 bbox = m_new_tile && !select_mode? m_new_tile->bbox : gbox;

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

bool TileMapEditor::onEscape() {
	m_is_selecting = false;
	return true;
}

bool TileMapEditor::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if((isKeyPressed(Key_lctrl) && key == 0) || key == 2) {
		m_view_pos -= getMouseMove();
		return true;
	}
	else if(key == 0) {
		m_selection = computeCursor(start, current);
		m_is_selecting = !is_final;
		if(is_final) {
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

				if(group_id != -1) {
					vector<int> entries;
					entries.reserve(m_tile_group->groupEntryCount(group_id));
					for(int n = 0; n < m_tile_group->entryCount(); n++)
						if(m_tile_group->entryGroup(n) == group_id)
							entries.push_back(n);

					int3 bbox = m_new_tile->bbox;

					for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
						for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
							int random_id = rand() % entries.size();
							const gfx::Tile *tile = m_tile_group->entryTile(entries[random_id]);

							try { m_tile_map->addTile(*tile, int3(x, m_selection.min.y, z)); }
							catch(...) { }
						}
				}
			}
			else if(m_mode == mAutoFilling && m_tile_group && m_new_tile) {
				int3 bbox = m_new_tile->bbox;

				for(int n = 0; n < m_tile_group->entryCount(); n++)
					m_tile_group->entryTile(n)->m_temp = n;

				for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
					for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
						int3 pos(x, m_selection.min.y, z);
						const TileInstance *neighbours[4] = {
							m_tile_map->at(pos + int3(0, 0, bbox.z)),
							m_tile_map->at(pos + int3(bbox.x, 0, 0)),
							m_tile_map->at(pos - int3(0, 0, bbox.z)),
							m_tile_map->at(pos - int3(bbox.x, 0, 0)) };

						int sides[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // -1: any, -2: error
						int ngroups[4];

						int soffset[4] = { 5, 7, 1, 3 };
						int doffset[4] = { 7, 1, 3, 5 };
						bool error = false;

						for(int n = 0; n < 4; n++) {
							int entry_id = neighbours[n]? m_tile_group->findEntry(neighbours[n]->m_tile) : -1;
							ngroups[n] = entry_id == -1? -1 : m_tile_group->entryGroup(entry_id);

							if(ngroups[n] != -1) {
								for(int s = 0; s < 3; s++) {
									int src_surf = m_tile_group->groupSurface(ngroups[n], (soffset[n] - s + 8) % 8);
									int dst_idx = (doffset[n] + s) % 8;

									if(sides[dst_idx] != -1 && sides[dst_idx] != src_surf)
										error = true;
									sides[dst_idx] = src_surf;
								}
							}
						}

						int any_count = 0;
						for(int n = 0; n < 8; n++) {
					//		printf("%d ", sides[n]);
							any_count += sides[n] == -1;
						}
						if(any_count > 4)
							error = true;
					//	printf(" err: %d\n", error?1 : 0);

						if(!error) {
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
								const gfx::Tile *tile = m_tile_group->entryTile(entries[random_id]);
								try { m_tile_map->addTile(*tile, int3(x, m_selection.min.y, z)); }
								catch(...) { }
							}
						}
					}
			}
		}

		return true;
	}

	return false;
}
	
void TileMapEditor::drawContents() const {
	ASSERT(m_tile_map);


	gfx::SceneRenderer renderer(clippedRect(), m_view_pos);
	m_tile_map->addToRender(renderer);
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

		drawGrid(box, m_grid_size, m_cursor_height);
	}
	
	if(m_new_tile && (m_mode == mPlacing || m_mode == mPlacingRandom) && m_new_tile) {
		int3 bbox = m_new_tile->bbox;
	
		for(int x = m_selection.min.x; x < m_selection.max.x; x += bbox.x)
			for(int z = m_selection.min.z; z < m_selection.max.z; z += bbox.z) {
				int3 pos(x, m_selection.min.y, z);

				bool collides = m_tile_map->isOverlapping(IBox(pos, pos + bbox));
				Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

				m_new_tile->draw(int2(worldToScreen(pos)), color);
				gfx::DTexture::bind0();
				gfx::drawBBox(IBox(pos, pos + bbox));
			}
//		m_tile_map->drawBoxHelpers(IBox(pos, pos + m_new_tile->bbox));
	}

//	m_tile_map->drawBoxHelpers(m_selection);
	DTexture::bind0();
	drawBBox(m_selection);
	
	lookAt(-clippedRect().min);
	gfx::PFont font = Font::mgr["times_24"];

	const char *mode_names[mCount] = {
		"selecting tiles",
		"placing new tiles",
		"placing new tiles (randomized)",
		"filling holes",
	};

	char text[64];
	snprintf(text, sizeof(text), "Cursor: (%d, %d, %d)  Mode: %s\n",
			m_selection.min.x, m_selection.min.y, m_selection.min.z, mode_names[m_mode]);
//	font->draw(int2(0, clippedRect().height() - 25), Color(255, 255, 255), text);
}
