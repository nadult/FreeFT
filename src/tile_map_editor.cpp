#include "tile_map_editor.h"
#include "gfx/device.h"

using namespace gfx;


TileMapEditor::TileMapEditor(IRect rect)
	:ui::Window(rect, Color(0, 0, 0)), m_show_grid(false), m_grid_size(3, 3), m_tile_map(0), m_new_tile(nullptr) {
	m_tile_group = nullptr;

	m_view_pos = int3(0, 0, 0);
	m_click_pos = int3(0, 0, 0);
	m_world_pos = int3(0, 0, 0);
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

	if(IsKeyDown(Key_kp_add))
		m_world_pos.y++;
	if(IsKeyDown(Key_kp_subtract))
		m_world_pos.y--;

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
				m_view_pos.xy += WorldToScreen(actions[n].offset * m_grid_size.x);
	}

	{
		int2 wp(ScreenToWorld(GetMousePos() + m_view_pos.xy - (int2)WorldToScreen(int3(0, m_world_pos.y, 0))));

		m_world_pos.x = wp.x;
		m_world_pos.z = wp.y;
	}
	Clamp(m_world_pos.y, 0, 255);

	if(m_show_grid) {
		m_world_pos.x -= m_world_pos.x % m_grid_size.x;
		m_world_pos.z -= m_world_pos.z % m_grid_size.y;
	}

	if(IsMouseKeyDown(0))
		m_click_pos = m_world_pos;

	m_selection = IBox(
		int3(Min(m_click_pos.x, m_world_pos.x), Min(m_click_pos.y, m_world_pos.y), Min(m_click_pos.z, m_world_pos.z)),
		int3(Max(m_click_pos.x, m_world_pos.x), Max(m_click_pos.y, m_world_pos.y), Max(m_click_pos.z, m_world_pos.z)) );
	if(m_world_pos.y == m_click_pos.y)
		m_selection.max.y++;

	if(m_new_tile) {
		if((!IsMouseKeyPressed(0) && !IsMouseKeyUp(0)) || m_selection.IsEmpty()) {
			m_selection.min = m_world_pos;
			m_selection.max = m_selection.min + (IsKeyPressed(Key_lshift)?
				int3(m_new_tile->bbox.x, 1, m_new_tile->bbox.z) : int3(m_grid_size.x, 1, m_grid_size.y));
		}

		if(IsMouseKeyUp(0)) {
			if(IsKeyPressed(Key_lshift))
				m_tile_map->Fill(*m_new_tile, m_selection);
			else {
				m_tile_map->Select(IBox(m_selection.min, m_selection.max + int3(0, 1, 0)),
						IsKeyPressed(Key_lctrl)? SelectionMode::add : SelectionMode::normal);
			}
		}
	}
	if(IsKeyDown('T') && m_tile_group) {
			
	}

	if(IsKeyPressed(Key_del))
		m_tile_map->DeleteSelected();
}
	
bool TileMapEditor::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if((IsKeyPressed(Key_lctrl) && key == 0) || key == 2) {
		m_view_pos.xy -= GetMouseMove();
		return true;
	}

	return false;
}

void TileMapEditor::drawContents() const {
	Assert(m_tile_map);
	int2 wsize = rect().Size();

	if(m_show_grid) {
		int2 p[4] = {
			ScreenToWorld(m_view_pos.xy),
			ScreenToWorld(m_view_pos.xy + wsize),
			ScreenToWorld(int2{m_view_pos.x, m_view_pos.y + wsize.y}),
			ScreenToWorld(int2{m_view_pos.x + wsize.x, m_view_pos.y}) };

		int2 min = Min(Min(p[0], p[1]), Min(p[2], p[3]));
		int2 max = Max(Max(p[0], p[1]), Max(p[2], p[3]));
		IBox box(min.x, 0, min.y, max.x, 0, max.y);
		drawGrid(box, m_grid_size, 0);//m_world_pos.y);
	}
	m_tile_map->Render(IRect(m_view_pos.xy, m_view_pos.xy + wsize));


	if(m_new_tile && IsKeyPressed(Key_lshift)) {
		m_tile_map->DrawPlacingHelpers(*m_new_tile, m_world_pos);
		m_tile_map->DrawBoxHelpers(IBox(m_world_pos, m_world_pos + m_new_tile->bbox));
	}

	m_tile_map->DrawBoxHelpers(m_selection);
	DTexture::Bind0();
	DrawBBox(m_selection);
}
