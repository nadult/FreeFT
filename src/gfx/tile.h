#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"

namespace gfx
{

	struct Tile: public Resource {
		void Serialize(Serializer &sr);
		void LoadDTexture();
		void Draw(int2 pos, Color color = Color(255, 255, 255)) const;

		string name;
		Texture texture;
		Ptr<DTexture> dTexture;

		IRect GetBounds() const;

		int2 offset;
		int3 bbox;

		static ResourceMgr<Tile> mgr;

		mutable uint m_temp;
	};

	float Similarity(const Tile &a, const Tile &b, int3 offsetBToA);

}


#endif
