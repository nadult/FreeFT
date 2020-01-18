// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/tile.h"

#include "gfx/drawing.h"
#include "gfx/scene_renderer.h"
#include <algorithm>
#include <fwk/gfx/texture.h>
#include <fwk/gfx/renderer2d.h>
#include <fwk/sys/file_stream.h>

namespace game
{

	static int s_frame_counter = 0;

	void Tile::setFrameCounter(int counter) {
		s_frame_counter = counter;
	}

	struct TypeName { TileId type; const char *name; } s_type_names[] = {
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

	Ex<void> TileFrame::load(FileStream &sr) {
		sr >> m_offset;
		m_texture = EX_PASS(PackedTexture::load(sr));
		return {};
	}
	void TileFrame::save(FileStream &sr) const {
		sr << m_offset;
	   	m_texture.save(sr);
	}

	int2 TileFrame::textureSize() const {
		return m_texture.size();
	}

	void TileFrame::cacheUpload(Texture &tex) const {
		DASSERT(m_palette_ref);
		m_texture.toTexture(tex, m_palette_ref->data(), m_palette_ref->size());
	}

	Texture TileFrame::texture() const {
		Texture out;
		m_texture.toTexture(out, m_palette_ref->data(), m_palette_ref->size());
		return out;
	}		

	PTexture TileFrame::deviceTexture(FRect &tex_rect) const {
		if(!isBind())
			bindToCache();
		return accessTexture(tex_rect);
	}
	
	Tile::Tile()
		:m_type_id(TileId::unknown), m_surface_id(SurfaceId::unknown), m_first_frame(&m_palette),
		m_see_through(false), m_walk_through(false), m_is_invisible(false) { }
		
	FlagsType Tile::flags() const {
		return	tileIdToFlag(m_type_id) |
				(m_see_through? (FlagsType)0 : Flags::occluding) |
				(m_walk_through? (FlagsType)0 : Flags::colliding);
	}
			
	template <class InputStream>
	Ex<void> Tile::legacyLoad(InputStream &sr, Str name) {
		ASSERT(sr.isLoading());

		sr.signature(Str("<tile>\0", 7));
		i16 type; sr >> type;

		if(type == 0x3031) {
			char dummy;
			sr >> dummy;
		}

		u8 size_x, size_y, size_z;
		sr.unpack(size_z, size_y, size_x);
		m_bbox = {size_x, size_y, size_z};
		
		i32 posX, posY;
		sr.unpack(posX, posY);

		m_offset = int2(posX, posY);
		i32 width, height;
		sr.unpack(width, height);

		u8 ttype, material, flags;
		sr.unpack(ttype, material, flags);
		m_type_id = ttype >= count<TileId>? TileId::unknown : (TileId)ttype;
		m_surface_id = material >= count<SurfaceId>? SurfaceId::unknown : (SurfaceId)material;
		m_see_through = flags & 8;
		m_is_invisible = name.find("Invisible Tile") != -1;
		if(m_is_invisible) {
			m_see_through = true;
			m_walk_through = true;
		}

		char unknown[3];
		int unk_size = type == '9'? 0 : type == '7'? 2 : type == '6'? 3 : 1;
		sr.loadData(span(unknown, unk_size));

		sr.signature(Str("<tiledata>\0001\0", 12));
		u8 dummy2;
		i32 zar_count;
		sr.unpack(dummy2, zar_count);

		Palette first_pal;

		for(int n = 0; n < zar_count; n++) {
			TileFrame frame(&m_palette);
			Palette palette;
			frame.m_texture = EX_PASS(PackedTexture::legacyLoad(sr, palette));
			i32 off_x, off_y;
			sr.unpack(off_x, off_y);
			frame.m_offset = int2(off_x, off_y);

			if(n == 0) {
				first_pal = palette;
				m_first_frame = frame;
			}
			else {
				ASSERT(palette == first_pal);
				m_frames.push_back(frame);
			}
		}

		m_palette = EX_PASS(Palette::legacyLoad(sr));
		ASSERT(first_pal == m_palette);

		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));
		updateMaxRect();
		
		ASSERT(sr.pos() == sr.size());
		return {};
	}

	Ex<void> Tile::load(FileStream &sr) {
		sr.signature("TILE");
		unsigned char type_id, surface_id;
		sr.unpack(type_id, surface_id, m_bbox, m_offset, m_see_through, m_walk_through, m_is_invisible);
		m_type_id = type_id >= count<TileId>? TileId::unknown : (TileId)type_id;
		m_surface_id = surface_id >= count<SurfaceId>? SurfaceId::unknown : (SurfaceId)surface_id;
		EXPECT(m_first_frame.load(sr));

		u32 size = 0;
		sr >> size;
		EXPECT(size <= 4096); // TODO: checks for size everywhere where needed?
		m_frames.resize(size);
		for(auto &frame : m_frames)
			EXPECT(frame.load(sr));
		m_palette = EX_PASS(Palette::load(sr));

		for(int n = 0; n < (int)m_frames.size(); n++)
			m_frames[n].m_palette_ref = &m_palette;
		updateMaxRect();
		return {};
	}
	
	template Ex<void> Tile::legacyLoad(MemoryStream&, Str);
	template Ex<void> Tile::legacyLoad(FileStream&, Str);

	void Tile::save(FileStream &sr) const {
		sr.signature("TILE");
		sr.pack(m_type_id, m_surface_id, m_bbox, m_offset, m_see_through, m_walk_through, m_is_invisible);
		DASSERT(!m_first_frame.m_texture.empty());

		m_first_frame.save(sr);
		sr << u32(m_frames.size());
		for(auto &frame : m_frames)
			frame.save(sr);
		m_palette.save(sr);
	}

	void Tile::draw(Renderer2D &out, const int2 &pos, Color col) const {
		const TileFrame &TileFrame = accessFrame(s_frame_counter);
		FRect tex_coords;
		auto tex = TileFrame.deviceTexture(tex_coords);
		out.addFilledRect(FRect(TileFrame.rect() + (pos - m_offset)), tex_coords, {tex, col});
	}

	void Tile::addToRender(SceneRenderer &renderer, const int3 &pos, Color color) const {
		const TileFrame &TileFrame = accessFrame(s_frame_counter);

		FRect tex_coords;
		auto tex = TileFrame.deviceTexture(tex_coords);
		renderer.add(tex, TileFrame.rect() - m_offset, (float3)pos, bboxSize(), color, tex_coords);
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
			m_max_rect = enclose(m_max_rect, m_frames[n].rect());
		m_max_rect -= m_offset;
	}

}
