#include "gfx/sprite.h"
#include <zlib.h>
#include <cstring>

namespace
{

	void Inflate(Serializer &sr, vector<char> &dest, int inSize)
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

	void Sprite::LoadFromSpr(Serializer &sr) {
		sr.signature("<sprite>", 9);

		i16 type; sr & type;
		u8 sizeX, sizeY, sizeZ; sr(sizeZ, sizeY, sizeX);
		m_bbox.x = sizeX;
		m_bbox.y = sizeY;
		m_bbox.z = sizeZ;

		i32 posX, posY; sr(posX, posY);
		m_offset = int2(posX, posY);
		
		m_offset -= WorldToScreen(int3(m_bbox.x, 0, m_bbox.z));

		char header[3];
		sr.data(header, sizeof(header));

		i32 seq_count; sr & seq_count;
		m_sequences.resize(seq_count);

		for(int n = 0; n < seq_count; n++) {
			Sequence &sequence = m_sequences[n];

			i16 frame_count, dummy1; sr(frame_count, dummy1);
			sequence.m_frames.resize(frame_count);
			for(int f = 0; f < frame_count; f++) {
				i16 frame_id; sr & frame_id;
				sequence.m_frames[f] = frame_id;
			}

			// Skip zeros
			sr.seek(sr.pos() + int(frame_count) * 4);

			i32 nameLen; sr & nameLen;
			DASSERT(nameLen >= 0 && nameLen <= 256);
			sequence.m_name.resize(nameLen);
			sr.data(&sequence.m_name[0], nameLen);
			i16 anim_id; sr & anim_id;
			sequence.m_anim_id = anim_id;
		}

		i32 anim_count; sr & anim_count;
		m_anims.resize(anim_count);

		for(int n = 0; n < anim_count; n++) {
			Animation &anim = m_anims[n];

			sr.signature("<spranim>\0001", 12);
			i32 offset; sr & offset;
			anim.m_offset = offset;

			i32 nameLen; sr & nameLen;
			DASSERT(nameLen >= 0 && nameLen <= 256);
			anim.m_name.resize(nameLen);
			sr.data(&anim.m_name[0], nameLen);
			
			i32 frame_count, dir_count;
			sr(frame_count, dir_count);

			int rect_count = frame_count * dir_count;
			anim.m_frame_count = frame_count;
			anim.m_dir_count = dir_count;
			anim.rects.resize(rect_count);

			for(int i = 0; i < rect_count; i++) {
				i32 l, t, r, b;
				sr(l, t, r, b);
				anim.rects[i] = Rect{l, t, r, b};
			}
		}

		for(int n = 0; n < anim_count; n++) {
			Animation &anim = m_anims[n];

			sr.seek(anim.m_offset);

			sr.signature("<spranim_img>", 14);
			i16 type; sr & type;

			if(type != '1' && type != '2')
				THROW("Unknown spranim_img type: %d", (int)type);
			anim.type = type;

			bool plainType = type == '1';
			vector<char> data;
			int size = (n == anim_count - 1? sr.size() : m_anims[n + 1].m_offset) - anim.m_offset - 16;

			if(plainType) {
				data.resize(size);
				sr.data(&data[0], size);
			}
			else {
				i32 plainSize = 0;
				sr & plainSize;
				Inflate(sr, data, size - 4);
				DASSERT((int)data.size() == plainSize);
			}

			PStream imgStream(new DataStream(data));
			Loader imgSr(imgStream);

			for(int l = 0; l < 4; l++) {
				i32 palSize; imgSr & palSize;
				anim.palettes[l].resize(palSize);
				imgSr.data(&anim.palettes[l][0], palSize * 4);
				for(int i = 0; i < palSize; i++)
					anim.palettes[l][i] = SwapBR(anim.palettes[l][i]);
			}

			int image_count = anim.m_frame_count * anim.m_dir_count * 4;
			anim.images.resize(image_count);
			anim.points.resize(image_count, int2(0, 0));

			for(int n = 0; n < image_count; n++) {
				DASSERT(imgSr.pos() < imgSr.size());
				char type; imgSr & type;

				if(type == 1) {
					i32 x, y; imgSr(x, y);
					anim.points[n] = int2(x, y);
					imgSr & anim.images[n];
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
			LoadFromSpr(sr);
		else
			THROW("Saving not supported");
	}

	Texture Sprite::Animation::getFrame(int frame_id, int dir_id) const {
		Texture out;
		int image_count = m_frame_count * m_dir_count;
		int2 size(0, 0);

		int ids[4];

		for(int l = 0; l < 4; l++)
			ids[l] = type == '1'?
				(frame_id * m_dir_count + dir_id) * 4 + l :
				dir_id * m_frame_count + l * image_count + frame_id;

		for(int l = 0; l < 4; l++) {
			int id = ids[l];
			size.x = Max(size.x, points[id].x + images[id].size.x);
			size.y = Max(size.y, points[id].y + images[id].size.y);
		}

		out.Resize(size.x, size.y);
		memset(&out(0, 0), 0, size.x * size.y * 4);

		for(int l = 0; l < 4; l++) {
			int id = ids[l];
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
		int out = 0;
		const Sequence &seq = m_sequences[seq_id];
		for(int n = 0; n < (int)seq.m_frames.size(); n++)
			if(seq.m_frames[n] >= 0)
				out++;
		return out;
	}

	Texture Sprite::getFrame(int seq_id, int frame_id, int dir_id, Rect *rect) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());
		DASSERT(frame_id >= 0 && frame_id < (int)m_sequences[seq_id].m_frames.size());

		const Sequence &seq = m_sequences[seq_id];

		for(int n = 0; n < (int)seq.m_frames.size(); n++)
			if(seq.m_frames[n] >= 0) {
				if(!frame_id) {
					frame_id = seq.m_frames[n];
					break;
				}
				frame_id--;
			}
		const Animation &anim = m_anims[seq.m_anim_id];
		DASSERT(dir_id < anim.m_dir_count);

		if(rect)
			*rect = anim.rects[frame_id * anim.m_dir_count + dir_id];

		return anim.getFrame(frame_id, dir_id);
	}

	int Sprite::findSequence(const char *name) const {
		for(int n = 0; n < (int)m_sequences.size(); n++)
			if(m_sequences[n].m_name == name)
				return n;
		return -1;
	}
		
	//TODO: samochody chyba maja wiecej dostepnych kierunkow...
	static int2 s_dirs[8] = {
		{-1, 0}, {-1, -1}, {0, -1}, {1, -1},
		{1, 0}, {1, 1}, {0, 1}, {-1, 1} };

	int Sprite::findDir(int dx, int dz) {
		dx = dx < 0? -1 : dx > 0? 1 : 0;
		dz = dz < 0? -1 : dz > 0? 1 : 0;

		for(int i = 0; i < COUNTOF(s_dirs); i++)
			if(s_dirs[i].x == dx && s_dirs[i].y == dz)
				return i;

		return 0;
	}

	void Sprite::printInfo() const {
		for(int a = 0; a < (int)m_anims.size(); a++) {
			const Sprite::Animation &anim = m_anims[a];
			printf("Anim %d: %s (%d*%d frames)\n", a, anim.m_name.c_str(),
					(int)anim.images.size() / anim.m_dir_count, anim.m_dir_count);
		}
		for(int s = 0; s < (int)m_sequences.size(); s++) {
			const Sprite::Sequence &seq = m_sequences[s];
			printf("Seq %d: %s (%d frames) -> anim: %d\n", s, seq.m_name.c_str(),
					(int)seq.m_frames.size(), seq.m_anim_id);
		}
	}

	ResourceMgr<Sprite> Sprite::mgr("../refs/sprites/", ".spr");

}
