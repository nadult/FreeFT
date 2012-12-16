#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"

namespace gfx
{

	// TODO: naming convention, attribute hiding
	struct Tile: public Resource {
		Tile();
		void serialize(Serializer &sr);
		bool testPixel(const int2 &pos) const;
		
		static ResourceMgr<Tile> mgr;

		string name;
		Texture texture;

		Ptr<DTexture> dTexture;
		FRect uvs;

		int width() const { return texture.width(); }
		int height() const { return texture.height(); }
		const IRect rect() const;

		void loadDeviceTexture();
		void bindTextureAtlas(PTexture, const int2 &pos);
		void draw(int2 pos, Color color = Color(255, 255, 255)) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

	protected:
		int2 m_offset;
		int3 m_bbox;
	};

	// dTexture & uvs attributes in tiles will be modified accordingly
	PTexture makeTileAtlas(const vector<gfx::Tile*>&);


	typedef Ptr<Tile> PTile;

}


#endif
