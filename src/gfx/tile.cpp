#include "gfx/tile.h"


namespace gfx
{

	void Tile::Serialize(Serializer &sr) {
		sr.Signature("<tile>\00010", 9);

		u8 dummy1, size_x, size_y, size_z;
		sr(dummy1, size_z, size_y, size_x);

		char unknown[20];
		sr.Data(unknown, 20);

		sr.Signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.LoadZAR(sr);	
	}

}
