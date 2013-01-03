#include "gfx/tile.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include <cstring>
#include <GL/gl.h>
#include <algorithm>


namespace gfx
{
	Tile::Tile() :m_tex_coords(0, 0, 1, 1), m_storage_mode(storage_none), m_cache_id(-1) { }

	Tile::~Tile() {
		if(m_storage_mode == storage_cache)
			cache.remove(m_cache_id);
	}

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
		new_texture->setSurface(tex);
		return new_texture;
	}

	PTexture Tile::deviceTexture() const {
		if(m_storage_mode == storage_cache)
			return cache.access(m_cache_id);
		if(m_storage_mode == storage_atlas || m_storage_mode == storage_single)
			return m_dev_texture;

		ASSERT(0);
		return nullptr;
	}
		
	int Tile::memorySize() const {
		return (int)(m_texture.memorySize() - sizeof(PackedTexture) + sizeof(Tile) + name.size());
	}

	void Tile::printInfo() const {
		printf("Tile %s:\n  Dimensions: %dx%d\nMemory: %.2f KB\nPalette: %d entries\n",
				name.c_str(), width(), height(), memorySize()/1024.0, m_texture.palette().size());
	}

	void Tile::storeSingle() {
		DASSERT(m_storage_mode == storage_none);
		m_dev_texture = loadTileTexture(this);
		m_storage_mode = storage_single;
	}

	void Tile::storeInCache() {
		DASSERT(m_storage_mode == storage_none);
		m_cache_id = cache.add(this);
		m_storage_mode = storage_cache;
	}

	void Tile::storeInAtlas(PTexture tex, const int2 &pos) {
		DASSERT(m_storage_mode == storage_none);
		m_dev_texture = tex;
		float2 mul(1.0f / (float)tex->width(), 1.0f / (float)tex->height());
		m_tex_coords = FRect(float2(pos) * mul, float2(pos + dimensions()) * mul);
		m_storage_mode = storage_atlas;
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
		return IRect(int2(0, 0), dimensions()) - m_offset;
	}

	void Tile::draw(const int2 &pos, Color col) const {
		PTexture tex = deviceTexture();
		tex->bind();
		drawQuad(pos - m_offset, dimensions(), m_tex_coords.min, m_tex_coords.max, col);
	}

	void Tile::addToRender(SceneRenderer &renderer, const int3 &pos, Color color) const {
		renderer.add(deviceTexture(), rect(), pos, bboxSize(), color, m_tex_coords);
	}

	bool Tile::testPixel(const int2 &pos) const {
		return m_texture.testPixel(pos + m_offset);
	}

	struct AtlasEntry {
		AtlasEntry(gfx::Tile *tile)
			:tile(tile), size(tile->width(), tile->height()), pos(-1, -1) { }
		bool operator<(const AtlasEntry &rhs) const { return size.y == rhs.size.y? size.x < rhs.size.x : size.y < rhs.size.y; }

		gfx::Tile *tile;
		int2 pos, size;
	};

	//TODO: move to some different file, make usable also for sprites?
	PTexture makeTileAtlas(const vector<gfx::Tile*> &tiles) {
		int max_size = 1024;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
		
		Texture atlas; {
			int width = max_size, height = max_size;
			int pixel_count = 0;
	
			for(int n = 0; n < (int)tiles.size(); n++)
				pixel_count += tiles[n]->width() * tiles[n]->height();

			while(width * height > pixel_count * 3 / 2) {
				if(height >= width)
					height /= 2;
				else
					width /= 2;
			}
			atlas.resize(width, height);
			memset(atlas.line(0), 0, atlas.width() * atlas.height() * sizeof(Color));
		}

		vector<AtlasEntry> entries(tiles.begin(), tiles.end());
		sort(entries.begin(), entries.end());
	
		int2 pos(0, 0);
		int max_y = 0;

		//TODO: better texture fitting
		for(int n = 0; n < (int)entries.size(); n++) {
			AtlasEntry &entry = entries[n];
			Texture tile_tex = entry.tile->texture();

			int2 tile_size = tile_tex.dimensions();

			if(tile_size.y + pos.y > atlas.height())
				continue;
			if(tile_size.x + pos.x > atlas.width()) {
				if(tile_size.x > atlas.width() || tile_size.y + max_y + pos.y > atlas.height())
					continue;
				pos = int2(0, pos.y + max_y);
				max_y = 0;
			}

			for(int y = 0; y < tile_size.y; y++)
				memcpy(atlas.line(pos.y + y) + pos.x, tile_tex.line(y), tile_size.x * sizeof(Color));
			max_y = max(max_y, tile_size.y);
			entry.pos = pos;
			pos.x += tile_size.x;
		}

		PTexture dev_atlas = new DTexture;
		dev_atlas->setSurface(atlas);
		int used_pixels = 0, missed = 0;

		for(int n = 0; n < (int)entries.size(); n++) {
			AtlasEntry &entry = entries[n];
			if(entry.pos == int2(-1, -1)) {
				entry.tile->storeSingle();
				missed++;
				continue;
			}
			entry.tile->storeInAtlas(dev_atlas, entry.pos);
			used_pixels += entry.tile->width() * entry.tile->height();
		}
		
		printf("Texture atlas size: %d x %d\nUtilization: %.2f %%\nTextures packed: %d / %d\n",
				atlas.width(), atlas.height(),
				float(used_pixels) * 100.0f / float(atlas.width() * atlas.height()),
				(int)entries.size() - missed, (int)entries.size());

		return dev_atlas;
	}

	ResourceMgr<Tile> Tile::mgr("data/tiles/", ".tile");
	TextureCache Tile::cache(24 * 1024 * 1024, loadTileTexture);

}
