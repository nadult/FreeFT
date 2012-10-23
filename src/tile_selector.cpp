#include "tile_selector.h"
#include "gfx/device.h"

using namespace gfx;

TileSelector::TileSelector(IRect rect) :Window(rect, Color(0, 0, 0)), m_offset(0, 0), m_tile_list(rect.Width(), 2) {
	m_tiles = nullptr;
	m_tile_group = nullptr;
	m_selection = nullptr;
}

void TileSelector::setSource(const vector<gfx::Tile> *tiles) {
	m_tiles = tiles;
	m_tile_group = nullptr;
	m_tile_list.m_entries.clear();

	for(int n = 0; n < (int)m_tiles->size(); n++)
		m_tile_list.add(&(*m_tiles)[n]);
	m_tile_list.update();
	m_selection = nullptr;
}
void TileSelector::setSource(const TileGroup* tile_group) {
	m_tiles = nullptr;
	m_tile_group = tile_group;
	m_tile_list.m_entries.clear();
	m_selection = nullptr;
}

void TileSelector::drawContents() const {
	for(int n = 0; n < m_tile_list.size(); n++) {
		const Tile *tile = m_tile_list[n].m_tile;
		tile->Draw(m_tile_list[n].m_pos - tile->GetBounds().min - m_offset);
	}
	
	if(m_selection) {	
		DTexture::Bind0();
		int2 pos = m_selection->m_pos - m_offset;
		DrawRect(IRect(pos, pos + m_selection->m_size));
	}
}
	
void TileSelector::onInput(int2 mouse_pos) {
	int wheel = GetMouseWheelMove();
	if(wheel)
		m_offset.y -= wheel * rect().Height() / 16;
	m_offset.y = Clamp(m_offset.y, 0, m_tile_list.m_height);
}

bool TileSelector::onMouseClick(int2 pos, int key) {
	if(key == 0 && !IsKeyPressed(Key_lctrl)) {
		m_selection = m_tile_list.find(pos + m_offset);
		return true;
	}

	return false;
}

bool TileSelector::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if(key == 2 || (IsKeyPressed(Key_lctrl) && key == 0)) {
		m_offset.y -= GetMouseMove().y;
		m_offset.y = Clamp(m_offset.y, 0, m_tile_list.m_height);
		return true;
	}

	return false;
}

