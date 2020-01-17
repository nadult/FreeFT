// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "gfx/packed_texture.h"
#include "gfx/texture_cache.h"
#include "game/base.h"

namespace game
{

	class TileFrame: public CachedTexture {
	public:
		TileFrame(const Palette *palette = nullptr) :m_palette_ref(palette) { }
		TileFrame(const TileFrame&);
		void operator=(const TileFrame&);
		void load(Stream&);
		void save(Stream&) const;

		virtual void cacheUpload(Texture&) const;
		virtual int2 textureSize() const;

		int2 dimensions() const { return m_texture.size(); }	
		Texture texture() const;
		PTexture deviceTexture(FRect &tex_rect) const;
	
		const IRect rect() const;

	protected:
		const Palette *m_palette_ref;
		PackedTexture m_texture;
		int2 m_offset;

		friend class Tile;
	};


	// TODO: naming convention, attribute hiding
	class Tile {
	public:
		Tile();
		Tile(const string &resource_name, Stream&);

		void legacyLoad(Stream &);
		void load(Stream &);
		void save(Stream &) const;
		
		FlagsType flags() const;

		TileId type() const { return m_type_id; }
		SurfaceId surfaceId() const { return m_surface_id; }

		const IRect rect(int frame_id) const;
		const IRect &rect() const { return m_max_rect; }

		static void setFrameCounter(int frame_counter);

		void draw(Renderer2D&, const int2 &pos, Color color = ColorId::white) const;
		void addToRender(SceneRenderer&, const int3 &pos, Color color = ColorId::white) const;
		bool testPixel(const int2 &pos) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

		bool isAnimated() const { return !m_frames.empty(); }
		bool isInvisible() const { return m_is_invisible; }
		
		const TileFrame &accessFrame(int frame_counter) const;
		int frameCount() const { return 1 + (int)m_frames.size(); }

		const string &resourceName() const { return m_resource_name; }
		void setResourceName(const string &name) { m_resource_name = name; }

	protected:
		void updateMaxRect();

		Palette m_palette;
		TileFrame m_first_frame;
		vector<TileFrame> m_frames;
		int2 m_offset;
		int3 m_bbox;
		IRect m_max_rect;
		string m_resource_name;

		TileId m_type_id;
		SurfaceId m_surface_id;
		bool m_see_through;
		bool m_walk_through;
		bool m_is_invisible;
	};


}
