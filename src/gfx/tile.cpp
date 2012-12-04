#include "gfx/tile.h"
#include <cstring>


namespace gfx
{

	void Tile::serialize(Serializer &sr) {
		sr.signature("<tile>", 7);
		i16 type; sr & type;

		if(type == 0x3031) {
			char dummy;
			sr & dummy;
		}

		u8 size_x, size_y, size_z;
		sr(size_z, size_y, size_x);
		m_bbox.x = size_x;
		m_bbox.y = size_y;
		m_bbox.z = size_z;
		
		i32 posX, posY; sr(posX, posY);
		m_offset = int2(posX, posY);

		i32 width, height;
		sr(width, height);

		char unknown[5];
		int unk_size = type == '9'? 3 : type == 0x3031? 4 : 5;
		sr.data(unknown, unk_size);

		sr.signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.loadZAR(sr);

		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));
	}

	void Tile::loadDTexture() {
		dTexture = new DTexture;
		dTexture->setSurface(texture);
	}

	IRect Tile::GetBounds() const {
		int2 size = texture.size();
		return IRect(0, 0, size.x, size.y) - m_offset;
	}

	void Tile::draw(int2 pos, Color col) const {
		if(!dTexture)
			((Tile*)this)->loadDTexture();
		dTexture->bind();

		int2 size = texture.size();
		drawQuad(pos.x - m_offset.x, pos.y - m_offset.y, size.x, size.y, col);
	}

	ResourceMgr<Tile> Tile::mgr("refs/tiles/", ".til");

}
