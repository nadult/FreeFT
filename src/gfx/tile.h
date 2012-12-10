#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"

namespace gfx
{

	struct Tile: public Resource {
		void serialize(Serializer &sr);
		void loadDTexture();
		void draw(int2 pos, Color color = Color(255, 255, 255)) const;
		bool testPixel(const int2 &pos) const;

		string name;
		Texture texture;
		Ptr<DTexture> dTexture;

		IRect GetBounds() const;

		int2 m_offset;
		int3 m_bbox;

		static ResourceMgr<Tile> mgr;

		mutable uint m_temp;
	};

	typedef Ptr<Tile> PTile;

}


#endif
