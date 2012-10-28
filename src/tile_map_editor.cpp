#include "tile_map_editor.h"
#include "gfx/device.h"

using namespace gfx;


TileMapEditor::TileMapEditor(IRect rect)
	:ui::Window(rect, Color(0, 0, 0)), m_show_grid(false), m_grid_size(3, 3), m_tile_map(0), m_new_tile(nullptr) {
	m_tile_group = nullptr;

	m_cursor_pos = int3(0, 0, 0);
	m_view_pos = int2(0, 0);
	m_is_selecting = false;
}

void TileMapEditor::drawGrid(const IBox &box, int2 node_size, int y) {
	DTexture::Bind0();

	//TODO: proper drawing when y != 0
	for(int x = box.min.x - box.min.x % node_size.x; x <= box.max.x; x += node_size.x)
		DrawLine(int3(x, y, box.min.z), int3(x, y, box.max.z), Color(255, 255, 255, 64));
	for(int z = box.min.z - box.min.z % node_size.y; z <= box.max.z; z += node_size.y)
		DrawLine(int3(box.min.x, y, z), int3(box.max.x, y, z), Color(255, 255, 255, 64));
}

void TileMapEditor::setTileMap(TileMap *new_tile_map) {
	//TODO: do some cleanup within the old tile map?
	m_tile_map = new_tile_map;
}

void TileMapEditor::onInput(int2 mouse_pos) {
	Assert(m_tile_map);

	m_cursor_pos = AsXZY(ScreenToWorld(mouse_pos + m_view_pos), m_cursor_pos.y);
	if(m_show_grid) {
		m_cursor_pos.x -= m_cursor_pos.x % m_grid_size.x;
		m_cursor_pos.z -= m_cursor_pos.z % m_grid_size.y;
	}
	if(IsKeyDown(Key_kp_add))
		m_cursor_pos.y++;
	if(IsKeyDown(Key_kp_subtract))
		m_cursor_pos.y--;

	if(IsKeyDown('G')) {
		if(m_show_grid) {
			if(m_grid_size.x == 3)
				m_grid_size = int2(6, 6);
			else if(m_grid_size.x == 6)
				m_grid_size = int2(9, 9);
			else {
				m_grid_size = int2(3, 3);
				m_show_grid = false;
			}
		}
		else
			m_show_grid = true;
	}

	{
		struct { KeyId key; int3 offset; } actions[] = {
			{ Key_kp_1, {  0, 0,  1 } }, 
			{ Key_kp_2, {  1, 0,  1 } },
			{ Key_kp_3, {  1, 0,  0 } },
			{ Key_kp_6, {  1, 0, -1 } },
			{ Key_kp_9, {  0, 0, -1 } },
			{ Key_kp_8, { -1, 0, -1 } },
			{ Key_kp_7, { -1, 0,  0 } },
			{ Key_kp_4, { -1, 0,  1 } } };
		
		for(int n = 0; n < COUNTOF(actions); n++)
			if(IsKeyDown(actions[n].key))
				m_view_pos += WorldToScreen(actions[n].offset * m_grid_size.x);
	}

	if(IsKeyDown('T') && m_tile_group) {
			
	}

	if(IsKeyPressed(Key_del))
		m_tile_map->DeleteSelected();
}
	
bool TileMapEditor::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if((IsKeyPressed(Key_lctrl) && key == 0) || key == 2) {
		m_view_pos -= GetMouseMove();
		return true;
	}
	else if(key == 0) {
		int3 start_pos = AsXZ(ScreenToWorld(start + m_view_pos));
		int3 end_pos = AsXZ(ScreenToWorld(current + m_view_pos));

		if(m_show_grid) {
			start_pos += AsXZ(m_grid_size / 2);
			end_pos += AsXZ(m_grid_size / 2);
			start_pos.x -= start_pos.x % m_grid_size.x;
			start_pos.z -= start_pos.z % m_grid_size.y;
			end_pos.x   -= end_pos.x   % m_grid_size.x;
			end_pos.z   -= end_pos.z   % m_grid_size.y;
		}

		m_selection = IBox(Min(start_pos, end_pos), Max(start_pos, end_pos));
		m_is_selecting = !is_final;
		if(is_final) {
			if(IsKeyPressed(Key_lshift) && m_new_tile)
				m_tile_map->Fill(*m_new_tile, IBox(m_selection.min, m_selection.max + int3(0, m_new_tile->bbox.y, 0)));
			else
				m_tile_map->Select(IBox(m_selection.min, m_selection.max + int3(0, 1, 0)),
						IsKeyPressed(Key_lctrl)? SelectionMode::add : SelectionMode::normal);
		}

		return true;
	}

	return false;
}
	
bool TileMapEditor::onMouseClick(int2 pos, int key, bool up) {
	if(key == 0 && IsKeyPressed(Key_lshift) && m_new_tile && up) {
		m_tile_map->Fill(*m_new_tile, IBox(m_cursor_pos, m_cursor_pos + m_new_tile->bbox));
		return true;
	}

	return false;
}

void TileMapEditor::drawContents() const {
	Assert(m_tile_map);

	IRect view_rect = clippedRect() - m_view_pos;
	LookAt(-view_rect.min);
	int2 wsize = view_rect.Size();

	if(m_show_grid) {
		int2 p[4] = {
			ScreenToWorld(m_view_pos + int2(0, 0)),
			ScreenToWorld(m_view_pos + int2(0, wsize.y)),
			ScreenToWorld(m_view_pos + int2(wsize.x, wsize.y)),
			ScreenToWorld(m_view_pos + int2(wsize.x, 0)) };

		int2 min = Min(Min(p[0], p[1]), Min(p[2], p[3]));
		int2 max = Max(Max(p[0], p[1]), Max(p[2], p[3]));
		IBox box(min.x, 0, min.y, max.x, 0, max.y);
		IBox bbox = m_tile_map->boundingBox();
		box = IBox(Max(box.min, bbox.min), Min(box.max, bbox.max));

		drawGrid(box, m_grid_size, m_cursor_pos.y);
	}
	m_tile_map->Render(IRect(m_view_pos, m_view_pos + wsize));


	if(m_new_tile && IsKeyPressed(Key_lshift) && !m_is_selecting) {
		int3 pos = m_is_selecting? m_selection.min : m_cursor_pos;
		m_tile_map->DrawPlacingHelpers(*m_new_tile, pos);
		m_tile_map->DrawBoxHelpers(IBox(pos, pos + m_new_tile->bbox));
	}

	if(m_is_selecting) {
		m_tile_map->DrawBoxHelpers(m_selection);
		DTexture::Bind0();
		DrawBBox(m_selection);
	}
}
