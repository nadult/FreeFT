/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_TILE_H
#define GAME_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"
#include "game/base.h"

namespace game
{

	class TileFrame: public gfx::CachedTexture {
	public:
		TileFrame(const gfx::Palette *palette = nullptr) :m_palette_ref(palette) { }
		TileFrame(const TileFrame&);
		void operator=(const TileFrame&);
		void load(Stream&);
		void save(Stream&) const;

		virtual void cacheUpload(gfx::Texture&) const;
		virtual int2 textureSize() const;

		int2 dimensions() const { return m_texture.dimensions(); }	
		gfx::Texture texture() const;
		gfx::PTexture deviceTexture(FRect &tex_rect) const;
	
		const IRect rect() const;

	protected:
		const gfx::Palette *m_palette_ref;
		gfx::PackedTexture m_texture;
		int2 m_offset;

		friend class Tile;
	};


	// TODO: naming convention, attribute hiding
	class Tile: public Resource {
	public:
		Tile();

		void legacyLoad(Stream &sr, const char *alternate_name = nullptr);
		void load(Stream &sr);
		void save(Stream &sr) const;
		
		static ResourceMgr<Tile> mgr;

		TileId::Type type() const { return m_type_id; }
		void setType(TileId::Type type) { m_type_id = type; }
		
		SurfaceId::Type surfaceId() const { return m_surface_id; }
		void setType(SurfaceId::Type id) { m_surface_id = id; }

		const IRect rect(int frame_id) const;
		const IRect &rect() const { return m_max_rect; }

		static void setFrameCounter(int frame_counter);

		void draw(const int2 &pos, Color color = Color::white) const;
		void addToRender(gfx::SceneRenderer&, const int3 &pos, Color color = Color::white) const;
		bool testPixel(const int2 &pos) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

		bool isAnimated() const { return !m_frames.empty(); }
		
		const TileFrame &accessFrame(int frame_counter) const;
		int frameCount() const { return 1 + (int)m_frames.size(); }

	protected:
		void updateMaxRect();

		gfx::Palette m_palette;
		TileFrame m_first_frame;
		vector<TileFrame> m_frames;
		int2 m_offset;
		int3 m_bbox;
		IRect m_max_rect;

		TileId::Type m_type_id;
		SurfaceId::Type m_surface_id;
	};

	typedef Ptr<Tile> PTile;

}


#endif
