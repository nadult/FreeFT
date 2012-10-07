#include "tile_selector.h"
#include "gfx/device.h"

using namespace gfx;

TileSelector::TileSelector(int2 res) :m_view(0, 0, res.x, res.y), m_offset(0), m_tile_id(0) {
	m_tiles = nullptr;
	m_tile_group = nullptr;
}

void TileSelector::loop() {
	if(IsKeyPressed(Key_lctrl) || IsMouseKeyPressed(2))
		m_offset += GetMouseMove().y;
	m_offset += GetMouseWheelMove() * m_view.Height() / 8;

	draw();
}

void TileSelector::draw() {
	int2 pos(0, m_offset);
	int maxy = 0;
	int2 mousePos = GetMousePos();
	LookAt(m_view.min);

	for(int n = 0, count = tileCount(); n < count; n++) {
		const Tile *tile = getTile(n);
		IRect bounds = tile->GetBounds();

		if(bounds.Width() + pos.x > m_view.Width() && pos.x != 0) {
			pos = int2(0, pos.y + maxy);
			if(pos.y >= m_view.Height())
				break;
			maxy = 0;
		}
		
		if(IRect(pos, pos + bounds.Size()).IsInside(mousePos))
			m_tile_id = n;

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

