#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"

namespace gfx
{

	class SceneRenderer;

	// TODO: naming convention, attribute hiding
	class Tile: public Resource, CachedTexture {
	public:
		void legacyLoad(Serializer &sr);
		void serialize(Serializer &sr);
		bool testPixel(const int2 &pos) const;
		
		static ResourceMgr<Tile> mgr;

		string name;

		int width() const { return m_texture.width(); }
		int height() const { return m_texture.height(); }
		const int2 dimensions() const { return m_texture.dimensions(); }

		int memorySize() const;
		void printInfo() const;

		const IRect rect() const;

		Texture texture() const;
		PTexture deviceTexture(FRect &tex_rect) const;

		void draw(const int2 &pos, Color color = Color::white) const;
		void addToRender(SceneRenderer&, const int3 &pos, Color color = Color::white) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

		virtual void cacheUpload(Texture&) const;
		virtual int2 textureSize() const { return dimensions(); }

	protected:
		PackedTexture m_texture;
		int2 m_offset;
		int3 m_bbox;
	};

	typedef Ptr<Tile> PTile;

}


#endif
