// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "tile_map.h"
#include "game/tile.h"
#include <climits>
#include <fwk/sys/stream.h>

//#define LOGGING

#ifndef LOGGING
#define printf(...)	
#endif

void zlibInflate(Stream &sr, vector<char> &dest, int inSize);

namespace game {

	void TileMap::legacyConvert(Stream &sr, Stream &out) {
		ASSERT(sr.isLoading());

		sr.signature("<world>", 8);
		u16 type;
		char dummy;

		i32 size1, size2;
		sr.unpack(type, dummy, size1, size2);

		printf("unc size: %d type: %x\n", size1, (int)type);

		vector<char> bytes;
		zlibInflate(sr, bytes, sr.size() - sr.pos());

		int map_manager = -1;
		for(int n = 0; n < (int)bytes.size(); n++)
			if(memcmp(bytes.data() + n, "<mapManager>", 13) == 0) {
				map_manager = n;
				break;
			}
	
		vector<Tile> tiles;

		{
			ASSERT(map_manager != -1);
			int offset = map_manager + 13;
			MemoryLoader dsr(bytes.data() + offset, bytes.size() - offset);

			clear();

			struct Header {
				u16 type;
				char temp[49];
				int something[3];
			} __attribute__((packed));
			Header header;
			dsr.loadData(&header, sizeof(header));

			int proto_count = 0, zero;
			dsr >> proto_count >> zero;
			proto_count--;

			vector<string> names;
			for(int n = 0; n < proto_count; n++) {
				int len = 0;
				char name[1024];
				dsr >> len;
				ASSERT(len < (int)sizeof(name));
				dsr.loadData(name, len);
				ASSERT(strncmp(name, "tiles/", 6) == 0 && strncmp(name + len - 4, ".til", 4) == 0);
				names.push_back(string(name + 6, name + len - 4));
			//	printf("name: %s\n", names.back().c_str());
			}

			struct TileParams {
				u8 bbox_z, bbox_y, bbox_x;
				int vals[5];
			} __attribute__((packed));

			vector<TileParams> tile_params;
			for(int n = 0; n < proto_count; n++) {
				dsr.signature("<tile>\00010", 10);
				TileParams tile;
				dsr >> tile;
				tile_params.push_back(tile);
			//	printf("TileParams [%d/%d]: %d %d %d\n", (int)tile_params.size(), proto_count, (int)tile.bbox_x, (int)tile.bbox_y, (int)tile.bbox_z);
			}
			printf("Proto count: %d\n", proto_count);

			tiles.resize(names.size());
			for(int n = 0; n < (int)names.size(); n++)
				tiles[n].setResourceName(names[n].c_str());
			
			char temp2[28];
			dsr.loadData(temp2, sizeof(temp2));

			int region_count = 0;
			dsr >> region_count;

			struct Instance {
				int3 pos, size;
				int tile_id;
			};
			vector<Instance> instances;

			IBox box(INT_MAX, INT_MAX, INT_MAX, INT_MIN, INT_MIN, INT_MIN);

			for(int n = 0; n < region_count; n++) {
				dsr.signature("<region>\0008", 11);
				int elem_count;

				dsr >> elem_count;
				for(int n = 0; n < elem_count; n++) {
					struct TInstance {
						short tile_id;
						short temp1;
						int min_pos[3], max_pos[3];
						int rest[7];
					};
	/*   Vals:
	min: 1 max: 134743069
	min: 0 max: 1470
	min: 81 max: 166
	min: 0 max: 2148
	min: 1 max: 1476
	min: 93 max: 167
	min: 2 max: 2154
	min: -154 max: 0
	min: -236 max: -5
	min: 1 max: 153
	min: -50 max: 7
	min: 0 max: 47906816
	min: 0 max: 725
	min: 1 max: 123505 */


					static_assert(sizeof(TInstance) == 56, "Wrong instance size");

					TInstance instance;
					dsr.loadData(&instance, sizeof(instance));
					int tile_id = (int)instance.tile_id - 1;
					if(tile_id < 0)
						continue;
					ASSERT(tile_id < (int)tiles.size());

					int3 pos(instance.min_pos[0], instance.min_pos[1], instance.min_pos[2]);
					int3 size = int3(instance.max_pos[0], instance.max_pos[1], instance.max_pos[2]) - pos;
					std::swap(pos.x, pos.z);
					std::swap(size.x, size.z);
					pos.x = -pos.x;
					pos.z = -pos.z;
					pos.x -= size.x;
					pos.z -= size.z;

					TileParams &params = tile_params[tile_id];
					int3 tile_psize = int3(params.bbox_x, params.bbox_y, params.bbox_z);
				//	int3 tile_size = tiles[tile_id]->bboxSize();
				//	if(size != tile_size || size != tile_psize)
				//		printf("%d %d %d | %d %d %d | %d %d %d\n", size.x, size.y, size.z, tile_size.x, tile_size.y, tile_size.z, tile_psize.x, tile_psize.y, tile_psize.z);
				//	ASSERT(tile_size == size && size == tile_psize);

					box = { vmin(box.min(), pos), vmax(box.max(), pos + size)};

					instances.push_back(Instance{pos, size, tile_id});
				}

			//	printf("Region: %d  elems: %d\n", n, elem_count);
			}
			printf("Regions: %d\noffset: %d / %d\n", region_count, (int)dsr.pos(), (int)dsr.size());

			printf("Map dimensions: (%d %d %d - %d %d %d)\n", box.x(), box.y(), box.z(), box.ex(), box.ey(), box.ez());
			resize(box.size().xz());

			printf("Constructing... from: %d elements\n", (int)instances.size());
			for(int n = 0; n < (int)instances.size(); n++) {
				Instance &inst = instances[n];

				float3 pos = float3(inst.pos - box.min());
				TileParams &params = tile_params[inst.tile_id];
				FBox bbox(pos, pos + float3(params.bbox_x, params.bbox_y, params.bbox_z));
				if(findAny(bbox) == -1)
					Grid::add(findFreeObject(), Grid::ObjectDef(&tiles[inst.tile_id], bbox, IRect(0, 0, 32, 32), 0xffffffff));
			}
		}

		XmlDocument doc;
		saveToXML(doc);
		out << doc;
		clear();

	//	Saver("mission.dec") & bytes;

		/*int tex_id = 0;
		for(int n = 0; n < (int)bytes.size() - 6; n++) {
			if(memcmp(bytes.data() + n, "<zar>", 6))
				continue;
			data_str->seek(n);

			try {
				Loader ldr(data_str);
				PackedTexture packed;
				packed.legacyLoad(ldr);
				Texture unpacked;
				packed.toTexture(unpacked);
				char name[100];
				sprintf(name, "texture%d.tga", tex_id++);
				printf("extracting texture: %d\n", tex_id - 1);
				Saver(name) & unpacked;
			}
			catch(...) { }
		}*/

	//	for(int n = 0; n < bytes.size(); n++)
	//		printf("%c", bytes[n] < 32? '.' : bytes[n]);
	}

}
