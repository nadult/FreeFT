/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/drawing.h"
#include "game/tile.h"
#include "gfx/scene_renderer.h"
#include <algorithm>

using namespace gfx;

namespace game
{

	static int s_frame_counter = 0;

	void Tile::setFrameCounter(int counter) {
		s_frame_counter = counter;
	}

	struct TypeName { TileId::Type type; const char *name; } s_type_names[] = {
		{ TileId::floor,	"_floor_" },
		{ TileId::wall,		"_wall_" },
		{ TileId::roof,		"_roof_" },
		{ TileId::object,	"_object_" },
		{ TileId::floor,	"_cap_" },
		{ TileId::floor,	"_stair_" },
		{ TileId::floor,	"_step_" },
	};

	const IRect TileFrame::rect() const {
		return IRect(m_offset, m_texture.size() + m_offset);
	}

	TileFrame::TileFrame(const TileFrame &rhs) :m_palette_ref(nullptr) {
		*this = rhs;
	}

	void TileFrame::operator=(const TileFrame &rhs) {
		m_texture = rhs.m_texture;
		m_offset = rhs.m_offset;
	}

	void TileFrame::load(Stream &sr) {
		sr >> m_offset >> m_texture;
	}
	void TileFrame::save(Stream &sr) const {
		sr << m_offset << m_texture;
	}

	int2 TileFrame::textureSize() const {
		return m_texture.size();
	}

	void TileFrame::cacheUpload(Texture &tex) const {
		m_texture.toTexture(tex, m_palette_ref->data(), m_palette_ref->size());
	}

	Texture TileFrame::texture() const {
		Texture out;
		m_texture.toTexture(out, m_palette_ref->data(), m_palette_ref->size());
		return out;
	}		

	PTexture TileFrame::deviceTexture(FRect &tex_rect) const {
		if(!getCache())
			bindToCache(TextureCache::main_cache);
		return accessTexture(tex_rect);
	}
	
	Tile::Tile()
		:m_type_id(TileId::unknown), m_surface_id(SurfaceId::unknown), m_first_frame(&m_palette),
		m_see_through(false), m_walk_through(false), m_is_invisible(false) { }
		
	Flags::Type Tile::flags() const {
		return	tileIdToFlag(m_type_id) |
				(m_see_through? (Flags::Type)0 : Flags::occluding) |
				(m_walk_through? (Flags::Type)0 : Flags::colliding);
	}
			
	void Tile::legacyLoad(Stream &sr, const char *name) {
		ASSERT(sr.isLoading());

		sr.signature("<tile>", 7);
		i16 type; sr >> type;

		if(type == 0x3031) {
			char dummy;
			sr >> dummy;
		}

		u8 size_x, size_y, size_z;
		sr.unpack(size_z, size_y, size_x);
		m_bbox.x = size_x;
		m_bbox.y = size_y;
		m_bbox.z = size_z;
		
		i32 posX, posY;
		sr.unpack(posX, posY);

		m_offset = int2(posX, posY);
		i32 width, height;
		sr.unpack(width, height);

		u8 ttype, material, flags;
		sr.unpack(ttype, material, flags);
		m_type_id = ttype >= TileId::count? TileId::unknown : (TileId::Type)ttype;
		m_surface_id = material >= SurfaceId::count? SurfaceId::unknown : (SurfaceId::Type)material;
		m_see_through = flags & 8;
		m_walk_through = flags & 1;
		m_is_invisible = strstr(sr.name(), "Invisible Tile") != nullptr;
		if(m_is_invisible) {
			m_see_through = true;
			m_walk_through = true;
		}

		char unknown[3];
		int unk_size = type == '9'? 0 : type == '7'? 2 : type == '6'? 3 : 1;
		sr.loadData(unknown, unk_size);

		sr.signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zar_count;
		sr.unpack(dummy2, zar_count);

		Palette first_pal;

		for(int n = 0; n < zar_count; n++) {
			TileFrame TileFrame(&m_palette);
			Palette palette;
			TileFrame.m_texture.legacyLoad(sr, palette);
			i32 off_x, off_y;
			sr.unpack(off_x, off_y);
			TileFrame.m_offset = int2(off_x, off_y);

			if(n == 0) {
				first_pal = palette;
				m_first_frame = TileFrame;
			}
			else {
				ASSERT(palette == first_pal);
				m_frames.push_back(TileFrame);
			}
		}

		m_palette.legacyLoad(sr);
		ASSERT(first_pal == m_palette);

		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));
		updateMaxRect();
		
		ASSERT(sr.pos() == sr.size());
	}

	void Tile::load(Stream &sr) {
		sr.signature("TILE", 4);
		sr.unpack(m_type_id, m_surface_id, m_bbox, m_offset, m_see_through, m_walk_through, m_is_invisible);
		ASSERT(TileId::isValid(m_type_id));
		sr >> m_first_frame >> m_frames >> m_palette;

		for(int n = 0; n < (int)m_frames.size(); n++)
			m_frames[n].m_palette_ref = &m_palette;
		updateMaxRect();
	}

	void Tile::save(Stream &sr) const {
		sr.signature("TILE", 4);
		sr.pack(m_type_id, m_surface_id, m_bbox, m_offset, m_see_through, m_walk_through, m_is_invisible);
		sr << m_first_frame << m_frames << m_palette;
	}

	void Tile::draw(const int2 &pos, Color col) const {
		const TileFrame &TileFrame = accessFrame(s_frame_counter);
		FRect tex_coords;

		PTexture tex = TileFrame.deviceTexture(tex_coords);
		tex->bind();
		IRect rect = TileFrame.rect();
		drawQuad(pos + rect.min - m_offset, rect.size(), tex_coords.min, tex_coords.max, col);
	}

	void Tile::addToRender(SceneRenderer &renderer, const int3 &pos, Color color) const {
		const TileFrame &TileFrame = accessFrame(s_frame_counter);

		FRect tex_coords;
		PTexture tex = TileFrame.deviceTexture(tex_coords);
		renderer.add(tex, TileFrame.rect() - m_offset, pos, bboxSize(), color, tex_coords);
	}

	bool Tile::testPixel(const int2 &pos) const {
		return accessFrame(s_frame_counter).m_texture.testPixel(pos + m_offset);
	}

	const TileFrame &Tile::accessFrame(int frame_counter) const {
		if(!m_frames.empty()) {
			frame_counter = frame_counter % frameCount();
			return frame_counter == 0? m_first_frame : m_frames[frame_counter - 1];
		}
		return m_first_frame;
	}
		
	const IRect Tile::rect(int frame_id) const {
		return accessFrame(frame_id).rect() - m_offset;
	}

	void Tile::updateMaxRect() {
		m_max_rect = m_first_frame.rect();
		for(int n = 0; n < (int)m_frames.size(); n++)
			m_max_rect = sum(m_max_rect, m_frames[n].rect());
		m_max_rect -= m_offset;
	}

	ResourceMgr<Tile> Tile::mgr("data/tiles/", ".tile");

}
