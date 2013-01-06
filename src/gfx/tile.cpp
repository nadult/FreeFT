#include "gfx/tile.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include <cstring>
#include <GL/gl.h>
#include <algorithm>


namespace gfx
{

	Texture Tile::texture() const {
		Texture out;
		m_texture.toTexture(out);
		return out;
	}

	static PTexture loadTileTexture(const void *ptr) {
		const Tile *tile = (const Tile*)ptr;
		DASSERT(tile);

		PTexture new_texture = new DTexture;
		Texture tex = tile->texture();
		new_texture->set(tex);
		return new_texture;
	}

	void Tile::cacheUpload(Texture &tex) const {
		m_texture.toTexture(tex);
	}

	PTexture Tile::deviceTexture(FRect &tex_rect) const {
		if(!getCache())
			bindToCache(TextureCache::main_cache);
		return accessTexture(tex_rect);
	}
		
	int Tile::memorySize() const {
		return (int)(m_texture.memorySize() - sizeof(PackedTexture) + sizeof(Tile) + name.size());
	}

	void Tile::printInfo() const {
		printf("Tile %s:\n  Dimensions: %dx%d\nMemory: %.2f KB\nPalette: %d entries\n",
				name.c_str(), width(), height(), memorySize()/1024.0, m_texture.palette().size());
	}

	void Tile::legacyLoad(Serializer &sr) {
		ASSERT(sr.isLoading());

		sr.signature("<tile>", 7);
		i16 type; sr & type;

		if(type == 0x3031) {
			char dummy;
			sr & dummy;
		}

		u8 size_x, size_y, size_z;
		sr(size_z, size_y, size_x);
		m_bbox.x = size_x;
		m_bbox.y = size_y;
		m_bbox.z = size_z;
		
		i32 posX, posY; sr(posX, posY);
		m_offset = int2(posX, posY);
		i32 width, height;
		sr(width, height);

		char unknown[5];
		int unk_size = type == '9'? 3 : type == '7'? 5 : type == '6'? 6 : 4;
		sr.data(unknown, unk_size);

		sr.signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zar_count;
		sr(dummy2, zar_count);
		//TODO: animation support in tiles (some tiles have more than one zar)

		m_texture.legacyLoad(sr);
		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));
	}

	void Tile::serialize(Serializer &sr) {
		sr.signature("TILE", 4);
		sr & m_texture;
		sr(m_bbox, m_offset);
	}

	const IRect Tile::rect() const {
		return IRect(-m_offset, dimensions() - m_offset);
	}

	void Tile::draw(const int2 &pos, Color col) const {
		FRect tex_coords;
		PTexture tex = deviceTexture(tex_coords);
		tex->bind();
		drawQuad(pos - m_offset, dimensions(), tex_coords.min, tex_coords.max, col);
	}

	void Tile::addToRender(SceneRenderer &renderer, const int3 &pos, Color color) const {
		FRect tex_coords;
		PTexture tex = deviceTexture(tex_coords);
		renderer.add(tex, rect(), pos, bboxSize(), color, tex_coords);
	}

	bool Tile::testPixel(const int2 &pos) const {
		return m_texture.testPixel(pos + m_offset);
	}

	ResourceMgr<Tile> Tile::mgr("data/tiles/", ".tile");

}
