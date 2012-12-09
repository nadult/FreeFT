#include "gfx/sprite.h"
#include <zlib.h>
#include <cstring>
#include <cstdio>

namespace
{

	void inflate(Serializer &sr, vector<char> &dest, int inSize)
	{
		enum { CHUNK = 16 * 1024 };

		int ret;
		unsigned have;
		z_stream strm;
		unsigned char in[CHUNK];
		unsigned char out[CHUNK];

		dest.clear();

		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		if(inflateInit(&strm) != Z_OK)
			THROW("Error while decompressing image data");

		/* decompress until deflate stream ends or end of file */
		do {
			strm.avail_in = inSize < CHUNK? inSize : CHUNK;
			sr.data(in, strm.avail_in);
			strm.next_in = in;
			inSize -= strm.avail_in;

			/* run inflate() on input until output buffer not full */
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				DASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */

				switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&strm);
					THROW("Z_MEM_ERROR while decompressing image data");
				}
				have = CHUNK - strm.avail_out;
				dest.resize(dest.size() + have);
				memcpy(&dest[dest.size() - have], out, have);
			} while (strm.avail_out == 0);

			/* done when inflate() says it's done */
		} while (ret != Z_STREAM_END && inSize);
		
		inflateEnd(&strm);

		if(ret != Z_STREAM_END)
			THROW("Error while decompressing image data");
	}

}

namespace gfx
{
	int Sprite::Frame::paramCount(char id) {
		if(id == ev_stop_anim || id == ev_jump_to_frame || id == ev_time_of_display)
			return 1;
		if(id == ev_fire)
			return 3;
		return 0;
	}

	void Sprite::Image::serialize(Serializer &sr) {
		sr.signature("<zar>", 6);

		char zar_type, dummy1, has_palette;
		u32 img_width, img_height;

		sr(zar_type, dummy1, img_width, img_height, has_palette);

		if(zar_type != 0x33 && zar_type != 0x34)
			THROW("Wrong zar type: %d", (int)zar_type);

		DASSERT(!has_palette);

		u8 defaultCol = 0;
		u32 rleSize; sr & rleSize;
		uint endPos = sr.pos() + rleSize;

		color.resize(img_width * img_height);
		alpha.resize(img_width * img_height, 255);
		size = int2(img_width, img_height);

		u8 *color_dst = &color[0], *end = &color.back();
		u8 *alpha_dst = &alpha[0];

		while(color_dst < end) {
			u8 cmd; sr & cmd;
			int n_pixels = cmd >> 2;
			int command = cmd & 3;

			if(command == 0)
				memset(alpha_dst, 0, n_pixels);
			else if(command == 1)
				sr.data(color_dst, n_pixels);
			else if(command == 2) {
				u8 buf[128];

				sr.data(buf, n_pixels * 2);
				for(int n = 0; n < n_pixels; n++) {
					color_dst[n] = buf[n * 2 + 0];
					alpha_dst[n] = buf[n * 2 + 1];
				}
			}
			else {
				sr.data(alpha_dst, n_pixels);
				memset(color_dst, defaultCol, n_pixels);
			}
				
			color_dst += n_pixels;
			alpha_dst += n_pixels;
		}

		sr.seek(endPos);
	}

	void Sprite::loadFromSpr(Serializer &sr) {
		sr.signature("<sprite>", 9);

		i16 type; sr & type;
		u8 sizeX, sizeY, sizeZ; sr(sizeZ, sizeY, sizeX);
		m_bbox.x = sizeX;
		m_bbox.y = sizeY;
		m_bbox.z = sizeZ;

		i32 posX, posY; sr(posX, posY);
		m_offset = int2(posX, posY);
		
		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));

		char header[3];
		sr.data(header, sizeof(header));

		i32 seq_count; sr & seq_count;
		m_sequences.resize(seq_count);

		for(int n = 0; n < seq_count; n++) {
			Sequence &sequence = m_sequences[n];

			i16 frame_count, dummy1; sr(frame_count, dummy1);
			vector<i16> frame_data(frame_count);
			sr.data(&frame_data[0], frame_count * sizeof(i16));
			for(int f = 0; f < (int)frame_data.size(); f++) {
				ASSERT(frame_data[f] >= -128 && frame_data[f] <= 127);
				int param_count = Frame::paramCount((char)frame_data[f]);
				if(param_count > 0) {
					frame_count -= param_count;
					f += param_count;
				}
			}

			sequence.frames.resize(frame_count);
			for(int f = 0, frame_id = 0; f < (int)frame_data.size(); f++) {
				int param_count = Frame::paramCount((char)frame_data[f]);
				sequence.frames[frame_id].id = frame_data[f];
				for(int p = 1; p <= param_count; p++) {
					ASSERT(frame_data[f + p] >= -128 && frame_data[f + p] <= 127);
					sequence.frames[frame_id].params[p - 1] = frame_data[f + p];
				}
				f += param_count;
				frame_id++;
			}

			// Skip zeros
			sr.seek(sr.pos() + int(frame_data.size()) * 4);

			i32 nameLen; sr & nameLen;
			DASSERT(nameLen >= 0 && nameLen <= 256);
			sequence.name.resize(nameLen);
			sr.data(&sequence.name[0], nameLen);
			i16 collection_id; sr & collection_id;
			sequence.collection_id = collection_id;
		}

		i32 collection_count; sr & collection_count;
		m_collections.resize(collection_count);

		for(int n = 0; n < collection_count; n++) {
			Collection &collection = m_collections[n];

			sr.signature("<spranim>\0001", 12);
			i32 offset; sr & offset;
			collection.m_offset = offset;

			i32 nameLen; sr & nameLen;
			DASSERT(nameLen >= 0 && nameLen <= 256);
			collection.name.resize(nameLen);
			sr.data(&collection.name[0], nameLen);
			
			i32 frame_count, dir_count;
			sr(frame_count, dir_count);

			int rect_count = frame_count * dir_count;
			collection.m_frame_count = frame_count;
			collection.m_dir_count = dir_count;
			collection.rects.resize(rect_count);

			for(int i = 0; i < rect_count; i++) {
				i32 l, t, r, b;
				sr(l, t, r, b);
				collection.rects[i] = IRect(l, t, r, b);
			}
		}

		for(int n = 0; n < collection_count; n++) {
			Collection &collection = m_collections[n];

			sr.seek(collection.m_offset);

			sr.signature("<spranim_img>", 14);
			i16 type; sr & type;

			if(type != '1' && type != '2')
				THROW("Unknown spranim_img type: %d", (int)type);
			collection.type = type;

			bool plainType = type == '1';
			vector<char> data;
			int size = (n == collection_count - 1? sr.size() : m_collections[n + 1].m_offset) -
						collection.m_offset - 16;

			if(plainType) {
				data.resize(size);
				sr.data(&data[0], size);
			}
			else {
				i32 plainSize = 0;
				sr & plainSize;
				inflate(sr, data, size - 4);
				DASSERT((int)data.size() == plainSize);
			}

			PStream imgStream(new DataStream(data));
			Loader imgSr(imgStream);

			for(int l = 0; l < 4; l++) {
				i32 palSize; imgSr & palSize;
				collection.palettes[l].resize(palSize);
				imgSr.data(&collection.palettes[l][0], palSize * 4);
				for(int i = 0; i < palSize; i++)
					collection.palettes[l][i] = swapBR(collection.palettes[l][i]);
			}

			int image_count = collection.m_frame_count * collection.m_dir_count * 4;
			collection.images.resize(image_count);
			collection.points.resize(image_count, int2(0, 0));

			for(int n = 0; n < image_count; n++) {
				DASSERT(imgSr.pos() < imgSr.size());
				char type; imgSr & type;

				if(type == 1) {
					i32 x, y; imgSr(x, y);
					collection.points[n] = int2(x, y);
					imgSr & collection.images[n];
				}
				else if(type == 0) { // empty image
				}
				else if(type == '<')
					break;
			}
		}
	}

	void Sprite::serialize(Serializer &sr) {
		if(sr.isLoading())
			loadFromSpr(sr);
		else
			THROW("Saving not supported");
	}

	Texture Sprite::Collection::getFrame(int frame_id, int dir_id) const {
		Texture out;
		int image_count = m_frame_count * m_dir_count;
		int2 size(0, 0);

		int ids[4];

		for(int l = 0; l < 4; l++)
			ids[l] = type == '1'?
				(frame_id * m_dir_count + dir_id) * 4 + l :
				dir_id * m_frame_count + l * image_count + frame_id;

		//TODO: there are still some bugs here
		for(int l = 0; l < 4; l++) {
			int id = ids[l];
//			ASSERT(id >= 0 && id <= (int)images.size()); //TODO
			if(id >= (int)images.size())
				continue;
			size.x = max(size.x, points[id].x + images[id].size.x);
			size.y = max(size.y, points[id].y + images[id].size.y);

		}

		out.resize(size.x, size.y);
		memset(&out(0, 0), 0, size.x * size.y * 4);

		for(int l = 0; l < 4; l++) {
			int id = ids[l];
//			ASSERT(id >= 0 && id <= (int)images.size()); //TODO
			if(id >= (int)images.size())
				continue;
			if(!images[id].size.x || !images[id].size.y)
				continue;

			int2 lsize = images[id].size;

			const Color *palette = &palettes[l][0];
			const u8 *colors = &images[id].color[0];
			const u8 *alphas = &images[id].alpha[0];
			Color *dst = &out(points[id].x, points[id].y);

			for(int y = 0; y < lsize.y; y++) {
				for(int x = 0; x < lsize.x; x++) {
					if(alphas[x]) {
						if(dst[x].a) {
							float4 dstCol = dst[x];
							float4 srcCol = palette[colors[x]];
							float4 col = (srcCol - dstCol) * float(alphas[x]) * (1.0f / 255.0f) + dstCol;
							dst[x] = Color(col.x, col.y, col.z, 1.0f);
						}
						else {
							dst[x] = palette[colors[x]];
							dst[x].a = alphas[x];
						}
					}
				}

				colors += lsize.x;
				alphas += lsize.x;
				dst += size.x;
			}
		}

		return out;
	}

	int Sprite::frameCount(int seq_id) const {
		return (int)m_sequences[seq_id].frames.size();
	}

	bool Sprite::isSequenceLooped(int seq_id) const {
		const Sequence &seq = m_sequences[seq_id];
		for(int n = 0; n < (int)seq.frames.size(); n++)
			if(seq.frames[n].id == ev_repeat_all)
				return true;
		return false;
	}

	Texture Sprite::getFrame(int seq_id, int frame_id, int dir_id, IRect *rect) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());
		DASSERT(frame_id >= 0 && frame_id < (int)m_sequences[seq_id].frames.size());

		/* TODO: powtarzalny bug na netbooku podczas strzelania z plazmowki
		 gfx/sprite.cpp:367: Assertion failed: frame_id >= 0 && frame_id < (int)m_sequences[seq_id].frames.size()

		Backtrace:
		./game(game::Entity::addToRender(gfx::SceneRenderer&) const+0x63) [0x808e753]
		./game(game::World::addToRender(gfx::SceneRenderer&)+0xa2) [0x808aec2]
		./game(safe_main(int, char**)+0xd20) [0x80a7b80]
		./game(main+0x25) [0x8061975]
		/lib/libc.so.6(__libc_start_main+0xfe) [0xb750ac2e]
		./game() [0x8061a55]  */


		const Sequence &seq = m_sequences[seq_id];
		frame_id = seq.frames[frame_id].id;
		DASSERT(frame_id >= 0);
		
		const Collection &collection = m_collections[seq.collection_id];
		DASSERT(dir_id < collection.m_dir_count);

		if(rect)
			*rect = collection.rects[frame_id * collection.m_dir_count + dir_id];

		return collection.getFrame(frame_id, dir_id);
	}

	int Sprite::findSequence(const char *name) const {
		for(int n = 0; n < (int)m_sequences.size(); n++)
			if(m_sequences[n].name == name)
				return n;
		return -1;
	}
		
	int Sprite::findDir(int seq_id, float radians) const {
//		DASSERT(radians >= 0.0f && radians < constant::pi * 2.0f);

		//TODO: wtf???
		float2 vec = angleToVector(radians);
		radians = vectorToAngle(-vec);
		
		int dir_count = dirCount(seq_id);
		float dir = radians * float(dir_count) * (0.5f / constant::pi) + 0.5f;
		return (int(dir) + dir_count) % dir_count;
	}

	void Sprite::printInfo() const {
		for(int a = 0; a < (int)m_collections.size(); a++) {
			const Sprite::Collection &collection = m_collections[a];
			printf("Collection %d: %s (%d*%d frames)\n", a, collection.name.c_str(),
					(int)collection.images.size() / collection.m_dir_count, collection.m_dir_count);
		}
		for(int s = 0; s < (int)m_sequences.size(); s++) {
			const Sprite::Sequence &seq = m_sequences[s];
			printf("Seq %d: %s (%d frames) -> collection #%d\n", s, seq.name.c_str(),
					(int)seq.frames.size(), seq.collection_id);
		}
	}

	void Sprite::printSequenceInfo(int seq_id) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());
		const Sequence &seq = m_sequences[seq_id];
		const Collection &collection = m_collections[seq.collection_id];

		printf("Sequence %d (%s) -> %d (%s)\n", seq_id, seq.name.c_str(), seq.collection_id, collection.name.c_str());
		for(int n = 0; n < (int)seq.frames.size(); n++)
			printf("frame #%d: %d\n", n, (int)seq.frames[n].id);
		printf("\n");
	}

	ResourceMgr<Sprite> Sprite::mgr("refs/sprites/", ".spr");

}
