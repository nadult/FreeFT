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
			ThrowException("Error while decompressing image data");

		/* decompress until deflate stream ends or end of file */
		do {
			strm.avail_in = inSize < CHUNK? inSize : CHUNK;
			sr.Data(in, strm.avail_in);
			strm.next_in = in;
			inSize -= strm.avail_in;

			/* run inflate() on input until output buffer not full */
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				Assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

				switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&strm);
					ThrowException("Z_MEM_ERROR while decompressing image data");
				}
				have = CHUNK - strm.avail_out;
				dest.resize(dest.size() + have);
				memcpy(&dest[dest.size() - have], out, have);
			} while (strm.avail_out == 0);

			/* done when inflate() says it's done */
		} while (ret != Z_STREAM_END && inSize);
		
		inflateEnd(&strm);

		if(ret != Z_STREAM_END)
			ThrowException("Error while decompressing image data");
	}

}


namespace gfx
{

	void Sprite::Image::Serialize(Serializer &sr) {
		sr.Signature("<zar>", 6);

		char zar_type, dummy1, has_palette;
		u32 img_width, img_height;

		sr(zar_type, dummy1, img_width, img_height, has_palette);

		if(zar_type != 0x33 && zar_type != 0x34)
			ThrowException("Wrong zar type: ", (int)zar_type);

		Assert(!has_palette);

		u8 defaultCol = 0;
		u32 rleSize; sr & rleSize;
		uint endPos = sr.Pos() + rleSize;

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
				sr.Data(color_dst, n_pixels);
			else if(command == 2) {
				u8 buf[128];

				sr.Data(buf, n_pixels * 2);
				for(int n = 0; n < n_pixels; n++) {
					color_dst[n] = buf[n * 2 + 0];
					alpha_dst[n] = buf[n * 2 + 1];
				}
			}
			else {
				sr.Data(alpha_dst, n_pixels);
				memset(color_dst, defaultCol, n_pixels);
			}
				
			color_dst += n_pixels;
			alpha_dst += n_pixels;
		}

		sr.Seek(endPos);
	}

	void Sprite::LoadFromSpr(Serializer &sr) {
		sr.Signature("<sprite>", 9);

		i16 type; sr & type;
		u8 sizeX, sizeY, sizeZ; sr(sizeZ, sizeY, sizeX);

		i32 posX, posY; sr(posX, posY);
		offset = int2(posX, posY);

		char header[3];
		sr.Data(header, sizeof(header));

		i32 numSeqs; sr & numSeqs;
		sequences.resize(numSeqs);

		for(int n = 0; n < numSeqs; n++) {
			Sequence &sequence = sequences[n];

			i16 numFrames, dummy1; sr(numFrames, dummy1);
			sequence.frames.resize(numFrames);
			for(int f = 0; f < numFrames; f++) {
				i16 frameId; sr & frameId;
				sequence.frames[f] = frameId;
			}

			// Skip zeros
			sr.Seek(sr.Pos() + int(numFrames) * 4);

			i32 nameLen; sr & nameLen;
			Assert(nameLen >= 0 && nameLen <= 256);
			sequence.name.resize(nameLen);
			sr.Data(&sequence.name[0], nameLen);
			i16 animId; sr & animId;
			sequence.animId = animId;
		}

		i32 numAnims; sr & numAnims;
		anims.resize(numAnims);

		for(int n = 0; n < numAnims; n++) {
			Animation &anim = anims[n];

			sr.Signature("<spranim>\0001", 12);
			i32 offset; sr & offset;
			anim.offset = offset;

			i32 nameLen; sr & nameLen;
			Assert(nameLen >= 0 && nameLen <= 256);
			anim.name.resize(nameLen);
			sr.Data(&anim.name[0], nameLen);
			
			i32 numFrames, numDirs;
			sr(numFrames, numDirs);

			int numRects = numFrames * numDirs;
			anim.numFrames = numFrames;
			anim.numDirs = numDirs;
			anim.rects.resize(numRects);

			for(int i = 0; i < numRects; i++) {
				i32 l, t, r, b;
				sr(l, t, r, b);
				anim.rects[i] = Rect{l, t, r, b};
			}
		}

		for(int n = 0; n < numAnims; n++) {
			Animation &anim = anims[n];

			sr.Seek(anim.offset);

			sr.Signature("<spranim_img>", 14);
			i16 type; sr & type;

			if(type != '1' && type != '2')
				ThrowException("Unknown spranim_img type: ", type);
			anim.type = type;

			bool plainType = type == '1';
			vector<char> data;
			int size = (n == numAnims - 1? sr.Size() : anims[n + 1].offset) - anim.offset - 16;

			if(plainType) {
				data.resize(size);
				sr.Data(&data[0], size);
			}
			else {
				i32 plainSize = 0;
				sr & plainSize;
				Inflate(sr, data, size - 4);
				Assert((int)data.size() == plainSize);
			}

			PStream imgStream(new DataStream(data));
			Loader imgSr(imgStream);

			for(int l = 0; l < 4; l++) {
				i32 palSize; imgSr & palSize;
				anim.palettes[l].resize(palSize);
				imgSr.Data(&anim.palettes[l][0], palSize * 4);
				for(int i = 0; i < palSize; i++)
					anim.palettes[l][i] = SwapBR(anim.palettes[l][i]);
			}

			int numImages = anim.numFrames * anim.numDirs * 4;
			anim.images.resize(numImages);
			anim.points.resize(numImages, int2(0, 0));

			for(int n = 0; n < numImages; n++) {
				Assert(imgSr.Pos() < imgSr.Size());
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

	Texture Sprite::Animation::GetFrame(int frameId, int dirId) const {
		Texture out;
		int numImg = numFrames * numDirs;
		int2 size(0, 0);

		int ids[4];

		for(int l = 0; l < 4; l++)
			ids[l] = type == '1'?
				(frameId * numDirs + dirId) * 4 + l :
				dirId * numFrames + l * numImg + frameId;

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

	int Sprite::NumFrames(int seqId) const {
		int out = 0;
		const Sequence &seq = sequences[seqId];
		for(int n = 0; n < seq.frames.size(); n++)
			if(seq.frames[n] >= 0)
				out++;
		return out;
	}

	Texture Sprite::GetFrame(int seqId, int frameId, int dirId, Rect *rect) const {
		const Sequence &seq = sequences[seqId];

		for(int n = 0; n < seq.frames.size(); n++)
			if(seq.frames[n] >= 0) {
				if(!frameId) {
					frameId = seq.frames[n];
					break;
				}
				frameId--;
			}
		const Animation &anim = anims[seq.animId];

		if(rect)
			*rect = anim.rects[frameId * anim.numDirs + dirId];

		return anim.GetFrame(frameId, dirId);
	}

}
