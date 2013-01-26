/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef GAME_TILE_H
#define GAME_TILE_H

#include "gfx/texture.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"
#include "game/enums.h"

namespace game
{

	class TileFrame: public gfx::CachedTexture {
	public:
		TileFrame(const gfx::Palette *palette = nullptr) :m_palette_ref(palette) { }
		TileFrame(const TileFrame&);
		void operator=(const TileFrame&);
		void serialize(Serializer&);

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

		void legacyLoad(Serializer &sr);
		void serialize(Serializer &sr);
		
		static ResourceMgr<Tile> mgr;

		TileId::Type type() const { return m_type; }
		void setType(TileId::Type type) { m_type = type; }

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

		TileId::Type m_type;
	};

	typedef Ptr<Tile> PTile;

}


#endif
