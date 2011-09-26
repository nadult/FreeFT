#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"

namespace gfx
{

	struct Tile {
		void Serialize(Serializer &sr);

		Texture texture;
	};

}


#endif
