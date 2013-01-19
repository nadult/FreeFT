/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
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

	// TODO: naming convention, attribute hiding
	class Tile: public Resource, gfx::CachedTexture {
	public:
		Tile();

		void legacyLoad(Serializer &sr);
		void serialize(Serializer &sr);
		bool testPixel(const int2 &pos) const;
		
		static ResourceMgr<Tile> mgr;

		const char *name() const { return m_name.empty()? nullptr : m_name.c_str(); }
		TileId::Type type() const { return m_type; }
		
		void setName(const char *str) { if(str) m_name = str; else m_name.clear(); }
		void setType(TileId::Type type) { m_type = type; }

		int width() const { return m_texture.width(); }
		int height() const { return m_texture.height(); }
		const int2 dimensions() const { return m_texture.dimensions(); }

		int memorySize() const;
		void printInfo() const;

		const IRect rect() const;

		gfx::Texture texture() const;
		gfx::PTexture deviceTexture(FRect &tex_rect) const;

		void draw(const int2 &pos, Color color = Color::white) const;
		void addToRender(gfx::SceneRenderer&, const int3 &pos, Color color = Color::white) const;

		const int3 &bboxSize() const { return m_bbox; }

		mutable uint m_temp;

		virtual void cacheUpload(gfx::Texture&) const;
		virtual int2 textureSize() const { return dimensions(); }

	protected:
		gfx::PackedTexture m_texture;
		int2 m_offset;
		int3 m_bbox;

		string m_name;
		TileId::Type m_type;
	};

	typedef Ptr<Tile> PTile;

}


#endif
