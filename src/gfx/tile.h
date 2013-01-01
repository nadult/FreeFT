#ifndef GFX_TILE_H
#define GFX_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"

namespace gfx
{

	class SceneRenderer;

	// TODO: naming convention, attribute hiding
	class Tile: public Resource {
	public:
		Tile();
		~Tile();
		Tile(const Tile&) = delete;
		void operator=(const Tile&) = delete;

		void legacyLoad(Serializer &sr);
		void serialize(Serializer &sr);
		bool testPixel(const int2 &pos) const;
		
		static ResourceMgr<Tile> mgr;
		static TextureCache cache;

		string name;

		int width() const { return m_texture.width(); }
		int height() const { return m_texture.height(); }
		const int2 dimensions() const { return m_texture.dimensions(); }
		int memorySize() const { return m_texture.memorySize() + (int)sizeof(Tile) + (int)name.size(); }

		const IRect rect() const;

		Texture texture() const;
		PTexture deviceTexture() const;

		//TODO: better names FFS...
		void storeSingle();
		void storeInCache();
		void storeInAtlas(PTexture, const int2 &pos);

		void draw(const int2 &pos, Color color = Color::white) const;
		void addToRender(SceneRenderer&, const int3 &pos, Color color = Color::white) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

		enum StorageMode {
			storage_none,
			storage_single,
			storage_cache,
			storage_atlas,
		};

		StorageMode storageMode() const { return m_storage_mode; }

//	protected:
		CompressedTexture m_texture;
		PTexture m_dev_texture;
		int m_cache_id;
		FRect m_tex_coords;
		StorageMode m_storage_mode;

		int2 m_offset;
		int3 m_bbox;
	};

	// dTexture & uvs attributes in tiles will be modified accordingly
	PTexture makeTileAtlas(const vector<gfx::Tile*>&);


	typedef Ptr<Tile> PTile;

}


#endif
