#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"

namespace gfx
{

	struct Tile {
		void Serialize(Serializer &sr);
		void LoadDTexture();
		void Draw(int2 pos);

		Texture texture;
		Ptr<DTexture> dTexture;

		int2 offset;
		int3 bbox;
	};

}


#endif
