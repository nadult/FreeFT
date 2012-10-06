#include "tile_map_editor.h"
#include "gfx/device.h"

using namespace gfx;


TileMapEditor::TileMapEditor(int2 res) :m_show_grid(false), m_grid_size(3, 3), m_tile_map(0) {
	m_view = IRect(0, 0, res.x, res.y);
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

void TileMapEditor::loop(const Tile *new_tile) {
	Assert(m_tile_map);

	if(IsKeyDown(Key_kp_add))
		m_world_pos.y++;
	if(IsKeyDown(Key_kp_subtract))
		m_world_pos.y--;

	if(IsKeyDown(Key_f2)) {
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
		int2 wp(ScreenToWorld(GetMousePos() + m_view.min - (int2)WorldToScreen(int3(0, m_world_pos.y, 0))));

		m_world_pos.x = wp.x;
		m_world_pos.z = wp.y;
	}
	Clamp(m_world_pos.y, 0, 255);

	if(m_show_grid) {
		m_world_pos.x -= m_world_pos.x % m_grid_size.x;
		m_world_pos.z -= m_world_pos.z % m_grid_size.y;
	}

	if((IsKeyPressed(Key_lctrl) && !IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
		m_view -= GetMouseMove();
	if(IsMouseKeyDown(0))
		m_click_pos = m_world_pos;

	m_selection = IBox(
		int3(Min(m_click_pos.x, m_world_pos.x), Min(m_click_pos.y, m_world_pos.y), Min(m_click_pos.z, m_world_pos.z)),
		int3(Max(m_click_pos.x, m_world_pos.x), Max(m_click_pos.y, m_world_pos.y), Max(m_click_pos.z, m_world_pos.z)) );
	if(m_world_pos.y == m_click_pos.y)
		m_selection.max.y++;

	if(new_tile) {
		if((!IsMouseKeyPressed(0) && !IsMouseKeyUp(0)) || m_selection.IsEmpty()) {
			m_selection.min = m_world_pos;
			m_selection.max = m_selection.min + (IsKeyPressed(Key_lshift)?
				int3(new_tile->bbox.x, 1, new_tile->bbox.z) : int3(m_grid_size.x, 1, m_grid_size.y));
		}

		if(IsMouseKeyUp(0)) {
			if(IsKeyPressed(Key_lshift))
				m_tile_map->Fill(*new_tile, m_selection);
			else {
				m_tile_map->Select(IBox(m_selection.min, m_selection.max + int3(0, 1, 0)),
						IsKeyPressed(Key_lctrl)? SelectionMode::add : SelectionMode::normal);
			}
		}
	}

	if(IsKeyPressed(Key_del))
		m_tile_map->DeleteSelected();

	draw(new_tile);
}

void TileMapEditor::draw(const Tile *new_tile) {
	Assert(m_tile_map);

	LookAt(m_view.min);

	if(m_show_grid) {
		int2 p[4] = {
			(int2)ScreenToWorld((float2)m_view.min),
			(int2)ScreenToWorld((float2)m_view.max),
			(int2)ScreenToWorld((float2){m_view.min.x, m_view.max.y}),
			(int2)ScreenToWorld((float2){m_view.max.x, m_view.min.y}) };

		int2 min = Min(Min(p[0], p[1]), Min(p[2], p[3]));
		int2 max = Max(Max(p[0], p[1]), Max(p[2], p[3]));
		IBox box(min.x, 0, min.y, max.x, 0, max.y);
		drawGrid(box, m_grid_size, 0);//m_world_pos.y);
	}
	m_tile_map->Render(m_view);


	if(new_tile && IsKeyPressed(Key_lshift)) {
		m_tile_map->DrawPlacingHelpers(*new_tile, m_world_pos);
		m_tile_map->DrawBoxHelpers(IBox(m_world_pos, m_world_pos + new_tile->bbox));
	}

	m_tile_map->DrawBoxHelpers(m_selection);
	DTexture::Bind0();
	DrawBBox(m_selection);
}
