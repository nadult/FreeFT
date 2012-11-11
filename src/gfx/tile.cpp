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
		bbox.x = size_x;
		bbox.y = size_y;
		bbox.z = size_z;
		
		i32 posX, posY; sr(posX, posY);
		offset = int2(posX, posY);

		i32 width, height;
		sr(width, height);

		char unknown[5];
		int unk_size = type == '9'? 3 : type == 0x3031? 4 : 5;
		sr.data(unknown, unk_size);

		sr.signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.LoadZAR(sr);

		offset -= WorldToScreen(int3(bbox.x, 0, bbox.z));
	}

	void Tile::LoadDTexture() {
		dTexture = new DTexture;
		dTexture->SetSurface(texture);
	}

	IRect Tile::GetBounds() const {
		int2 size = texture.Size();
		return IRect(0, 0, size.x, size.y) - offset;
	}

	void Tile::Draw(int2 pos, Color col) const {
		dTexture->Bind();

		int2 size = texture.Size();
		DrawQuad(pos.x - offset.x, pos.y - offset.y, size.x, size.y, col);
	}

	float Similarity(const Tile &a, const Tile &b, int3 offset) {
		return 0.0f;	
	}

	ResourceMgr<Tile> Tile::mgr("", "");

}
