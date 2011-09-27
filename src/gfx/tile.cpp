#include "gfx/tile.h"


namespace gfx
{

	void Tile::Serialize(Serializer &sr) {
		sr.Signature("<tile>", 7);
		i16 type; sr & type;

		u8 dummy1, size_x, size_y, size_z;
		sr(size_z, size_y, size_x);

		i32 posX, posY; sr(posX, posY);
		offset = int2(posX, posY);

		char unknown[13];
		sr.Data(unknown, type == '9'? 11 : 13);

		sr.Signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.LoadZAR(sr);	
	}

}
