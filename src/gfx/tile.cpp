#include "gfx/tile.h"


namespace gfx
{

	void Tile::Serialize(Serializer &sr) {
		sr.Signature("<tile>", 7);
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

		char unknown[13];
		sr.Data(unknown, type == '9'? 11 : type == 0x3031? 12 : 13);

		sr.Signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.LoadZAR(sr);	
	}

	void Tile::LoadDTexture() {
		dTexture = new DTexture;
		dTexture->SetSurface(texture);
	}

	void Tile::Draw(int2 pos) {
		dTexture->Bind();

		int2 size = texture.Size();
		DrawQuad(pos.x - offset.x, pos.y - offset.y, size.x, size.y);
	}

}
