#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"

namespace gfx
{

	class SceneRenderer;

	// TODO: naming convention, attribute hiding
	struct Tile: public Resource {
		Tile();
		void serialize(Serializer &sr);
		bool testPixel(const int2 &pos) const;
		
		static ResourceMgr<Tile> mgr;

		string name;

		int width() const { return m_texture.width(); }
		int height() const { return m_texture.height(); }
		const int2 size() const { return m_texture.size(); }

		const IRect rect() const;

		const Texture &texture() const { return m_texture; }
		PTexture deviceTexture() const { return m_dev_texture; }
		void loadDeviceTexture();

		void bindTextureAtlas(PTexture, const int2 &pos);
		void draw(const int2 &pos, Color color = Color::white) const;
		void addToRender(SceneRenderer&, const int3 &pos, Color color = Color::white) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

	protected:
		Texture m_texture;
		Ptr<DTexture> m_dev_texture;
		FRect m_tex_coords;

		int2 m_offset;
		int3 m_bbox;
	};

	// dTexture & uvs attributes in tiles will be modified accordingly
	PTexture makeTileAtlas(const vector<gfx::Tile*>&);


	typedef Ptr<Tile> PTile;

}


#endif
