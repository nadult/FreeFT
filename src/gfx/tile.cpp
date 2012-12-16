#include "gfx/tile.h"
#include "gfx/device.h"
#include <cstring>
#include <GL/gl.h>
#include <algorithm>


namespace gfx
{
	Tile::Tile() :uvs(0, 0, 1, 1) { }

	void Tile::serialize(Serializer &sr) {
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
		int unk_size = type == '9'? 3 : type == 0x3031? 4 : 5;
		sr.data(unknown, unk_size);

		sr.signature("<tiledata>\0001", 12);
		u8 dummy2;
		i32 zarCount;
		sr(dummy2, zarCount);

		texture.loadZAR(sr);

		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));
	}

	void Tile::loadDeviceTexture() {
		dTexture = new DTexture;
		dTexture->setSurface(texture);
		uvs = FRect(0, 0, 1, 1);
	}

	void Tile::bindTextureAtlas(PTexture tex, const int2 &pos) {
		dTexture = tex;
		float2 mul(1.0f / (float)tex->width(), 1.0f / (float)tex->height());
		uvs = FRect(float2(pos) * mul, float2(pos + texture.size()) * mul);
	}

	const IRect Tile::rect() const {
		return IRect(int2(0, 0), texture.size()) - m_offset;
	}

	void Tile::draw(int2 pos, Color col) const {
		DASSERT(dTexture);
		dTexture->bind();
		drawQuad(pos - m_offset, texture.size(), uvs.min, uvs.max, col);
	}

	bool Tile::testPixel(const int2 &pos) const {
		return texture.testPixel(pos + m_offset);
	}

	struct AtlasEntry {
		AtlasEntry(gfx::Tile *tile)
			:tile(tile), size(tile->width(), tile->height()), pos(-1, -1) { }
		bool operator<(const AtlasEntry &rhs) const { return size.y == rhs.size.y? size.x < rhs.size.x : size.y < rhs.size.y; }

		gfx::Tile *tile;
		int2 pos, size;
	};

	PTexture makeTileAtlas(const vector<gfx::Tile*> &tiles) {
		int max_size = 1024;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
		
		Texture atlas; {
			int width = max_size, height = max_size;
			int pixel_count = 0;
	
			for(int n = 0; n < (int)tiles.size(); n++)
				pixel_count += tiles[n]->texture.width() * tiles[n]->texture.height();

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

		for(int n = 0; n < (int)entries.size(); n++) {
			AtlasEntry &entry = entries[n];
			int2 tile_size = entry.tile->texture.size();
			if(tile_size.y + pos.y > atlas.height())
				continue;
			if(tile_size.x + pos.x > atlas.width()) {
				if(tile_size.x > atlas.width() || tile_size.y + max_y + pos.y > atlas.height())
					continue;
				pos = int2(0, pos.y + max_y);
				max_y = 0;
			}

			for(int y = 0; y < tile_size.y; y++)
				memcpy(atlas.line(pos.y + y) + pos.x, entry.tile->texture.line(y), tile_size.x * sizeof(Color));
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
				missed++;
				continue;
			}
			entry.tile->bindTextureAtlas(dev_atlas, entry.pos);
			used_pixels += entry.tile->width() * entry.tile->height();
		}
		
		printf("Texture atlas size: %d x %d\nUtilization: %.2f %%\nTextures packed: %d / %d\n",
				atlas.width(), atlas.height(),
				float(used_pixels) * 100.0f / float(atlas.width() * atlas.height()),
				(int)entries.size() - missed, (int)entries.size());

		return dev_atlas;
	}

	ResourceMgr<Tile> Tile::mgr("refs/tiles/", ".til");

}
