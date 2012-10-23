#include "tile_selector.h"
#include "gfx/device.h"

using namespace gfx;

TileSelector::TileSelector(IRect rect) :Window(rect, Color(0, 0, 0)), m_offset(0), m_tile_id(0) {
	m_tiles = nullptr;
	m_tile_group = nullptr;
}

void TileSelector::drawContents() const {
	int2 pos(0, m_offset);
	int maxy = 0;
//	int2 mousePos = GetMousePos();

	for(int n = 0, count = tileCount(); n < count; n++) {
		const Tile *tile = getTile(n);
		IRect bounds = tile->GetBounds();

		if(bounds.Width() + pos.x > rect().Width() && pos.x != 0) {
			pos = int2(0, pos.y + maxy);
			if(pos.y >= rect().Height())
				break;
			maxy = 0;
		}
		
//		if(IRect(pos, pos + bounds.Size()).IsInside(mousePos))
//			m_tile_id = n;

		if(pos.y + bounds.Height() >= 0) {
			tile->Draw(pos - bounds.min);
			DTexture::Bind0();

			if(m_tile_id == (int)n)
				DrawRect(IRect(pos, pos + bounds.Size()));
		}

		pos.x += bounds.Width() + 2;
		maxy = Max(maxy, bounds.Height() + 2);
	}
}

bool TileSelector::onMouseClick(int2 pos, int key) {
	return false;
}

bool TileSelector::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if(key == 2 || (IsKeyPressed(Key_lctrl) && key == 0))
		m_offset += GetMouseMove().y;
	m_offset += GetMouseWheelMove() * rect().Height() / 8;
	return true;
}

