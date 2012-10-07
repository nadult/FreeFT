#include "tile_group_editor.h"
#include "gfx/device.h"

using namespace gfx;

/*
float Compare(const Tile &a, const Tile &b) {
	int2 offset = b.offset - a.offset - int2(WorldToScreen(float3(a.bbox.x, 0.0f, 0.0f)));
	int2 aSize = a.texture.Size(), bSize = b.texture.Size();
	int2 size(Min(aSize.x, bSize.x), Min(aSize.y, bSize.y));

//	printf("Size: %dx%d\n", aSize.x, aSize.y);
//	printf("Offset: %dx%d\n", offset.x, offset.y);
	
	int2 min(offset.x < 0? -offset.x : 0, offset.y < 0? -offset.y : 0);
	int2 max(offset.x > 0? size.x - offset.x : size.x, offset.y > 0? size.y - offset.y : size.y);
	
	int missed = 0, matched = 0;
	for(int y = min.y; y < max.y; y++)
		for(int x = min.x; x < max.x; x++) {
			Color aPix = a.texture(x, y);
			Color bPix = b.texture(x + offset.x, y + offset.y);
				
			if(!aPix.a || !bPix.a)
				missed++;
			else {
				int rdist = abs((int)aPix.r - (int)bPix.r);
				int gdist = abs((int)aPix.g - (int)bPix.g);
				int bdist = abs((int)aPix.b - (int)bPix.b);

				int dist = Max(rdist, Max(gdist, bdist));
				if(dist < 20)
					matched++;	
			}
		}

	char text[256];
	snprintf(text, sizeof(text), "size: %dx%d  off: %dx%d  matches: %d/%d", size.x, size.y, offset.x, offset.y,
				matched, (max.x - min.x) * (max.y - min.y) - missed);
	font.Draw(text);

	return float(matched) / float((max.x - min.x) * (max.y - min.y));
}*/

TileGroupEditor::TileGroupEditor(int2 res) :m_view(0, 0, res.x, res.y), m_offset(0), m_tile_id(0) {
	cmpId[0] = cmpId[1] = 0;
	m_tiles = nullptr;
	m_tile_group = nullptr;
}

void TileGroupEditor::loop() {
	if(IsKeyPressed(Key_lctrl) || IsMouseKeyPressed(2))
		m_offset += GetMouseMove().y;
	m_offset += GetMouseWheelMove() * m_view.Height() / 8;
	LookAt({0, 0});
	draw();

	LookAt({0, 0});
	//fontTex.Bind();
	//font.SetSize(int2(35, 25));

	//font.SetPos(int2(5, 65));
	//Compare(tiles[cmpId[0]], tiles[cmpId[1]]);
}

void TileGroupEditor::draw() {
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
		
		if(IRect(pos, pos + bounds.Size()).IsInside(mousePos)) {
			m_tile_id = n;
			if(IsKeyPressed('1'))
				cmpId[0] = n;
			if(IsKeyPressed('2'))
				cmpId[1] = n;
		}

		if(pos.y + bounds.Height() >= 0) {
			tile->Draw(pos - bounds.min);
			DTexture::Bind0();

			if(m_tile_id == (int)n)
				DrawRect(IRect(pos, pos + bounds.Size()));
			if(cmpId[0] == (int)n)
				DrawRect(IRect(pos, pos + bounds.Size()), Color(255, 0, 0));
			if(cmpId[1] == (int)n)
				DrawRect(IRect(pos, pos + bounds.Size()), Color(0, 255, 0));
		}

		pos.x += bounds.Width() + 2;
		maxy = Max(maxy, bounds.Height() + 2);
	}
}

