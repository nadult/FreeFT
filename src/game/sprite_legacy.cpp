// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/sprite.h"
#include <zlib.h>
#include <fwk/sys/file_stream.h>

// source: http://www.zlib.net/zlib_how.html
template <class InputStream>
Ex<void> zlibInflate(InputStream &sr, vector<char> &dest, int inSize) {
	int chunk_size = 16 * 1024;

	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[chunk_size];
	unsigned char out[chunk_size];

	dest.clear();

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	if(inflateInit(&strm) != Z_OK)
		return ERROR("Error while decompressing image data");

	/* decompress until deflate stream ends or end of file */
	do {
		strm.avail_in = inSize < chunk_size? inSize : chunk_size;
		sr.loadData(span(in, strm.avail_in));
		strm.next_in = in;
		inSize -= strm.avail_in;

		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = chunk_size;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			DASSERT(ret != Z_STREAM_ERROR);  /* state not clobbered */

			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
				[[fallthrough]];
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				inflateEnd(&strm);
				return ERROR("Z_MEM_ERROR while decompressing image data");
			}
			have = chunk_size - strm.avail_out;
			dest.resize(dest.size() + have);
			memcpy(&dest[dest.size() - have], out, have);
		} while (strm.avail_out == 0);
		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END && inSize);
	
	inflateEnd(&strm);

	if(ret != Z_STREAM_END)
		return ERROR("Error while decompressing image data");
	return {};
}

namespace game
{
	namespace {
		struct Collection {
			static constexpr int layer_count = 4;
			static_assert((int)layer_count <= (int)Sprite::layer_count, "Wrong layer count");

			string name;
			vector<IRect> rects;
			vector<Color> palettes[layer_count];
			vector<PackedTexture> images;
			vector<int2> points;
			int frame_count, dir_count, offset;
			int type;

			void getLayerIndices(int frame_id, int dir_id, int *layer_indices) const {
				int image_count = frame_count * dir_count;

				for(int l = 0; l < layer_count; l++)
					layer_indices[l] = type == '1'?
						(frame_id * dir_count + dir_id) * 4 + l :
						dir_id * frame_count + l * image_count + frame_id;
			}

		};

	}

	template <class InputStream>
	Ex<void> Sprite::legacyLoad(InputStream &sr, Str) {
		ASSERT(sr.isLoading());
		sr.signature(Str("<sprite>\0", 9));
		
		clear();

		i16 type; sr >> type;
		u8 sizeX, sizeY, sizeZ; sr.unpack(sizeZ, sizeY, sizeX);
		m_bbox.x = sizeX;
		m_bbox.y = sizeY;
		m_bbox.z = sizeZ;

		i32 posX, posY; sr.unpack(posX, posY);
		m_offset = int2(posX, posY);
		
		m_offset -= worldToScreen(int3(m_bbox.x, 0, m_bbox.z));

		char header[3];
		sr.loadData(header);

		i32 seq_count; sr >> seq_count;
		m_sequences.resize(seq_count);

		vector<int> seq2col(seq_count);

		for(int n = 0; n < seq_count; n++) {
			Sequence &sequence = m_sequences[n];

			i16 frame_count, dummy1; sr.unpack(frame_count, dummy1);
			i16 frame_data[frame_count];
			sr.loadData(span(frame_data, frame_count));
			sequence.frame_count = frame_count;
			sequence.first_frame = (int)m_frames.size();

			for(int f = 0; f < frame_count; f++) {
				Frame frame;
				frame.id = frame_data[f];

				int param_count = Frame::paramCount((char)frame_data[f]);
				for(int p = 0; p < param_count; p++)
					frame.params[p] = frame_data[f + p + 1];

				f += param_count;
				sequence.frame_count -= param_count;

				m_frames.push_back(frame);
			}


			i32 frame_data2[frame_count];
			sr.loadData(span(frame_data2, frame_count));

			i32 nameLen; sr >> nameLen;
			ASSERT(nameLen >= 0 && nameLen <= 256);

			sequence.name.resize(nameLen);
			sr.loadData(span(&sequence.name[0], nameLen));
			
			i16 collection_id; sr >> collection_id;
			seq2col[n] = collection_id;
		}

		i32 collection_count; sr >> collection_count;
		vector<Collection> collections(collection_count);

		for(int n = 0; n < collection_count; n++) {
			Collection &collection = collections[n];

			sr.signature(Str("<spranim>\0001\0", 12));
			i32 offset; sr >> offset;
			collection.offset = offset;

			i32 nameLen; sr >> nameLen;
			ASSERT(nameLen >= 0 && nameLen <= 256);
			collection.name.resize(nameLen);
			sr.loadData(span(&collection.name[0], nameLen));
			
			i32 frame_count, dir_count;
			sr.unpack(frame_count, dir_count);

			int rect_count = frame_count * dir_count;
			collection.frame_count = frame_count;
			collection.dir_count = dir_count;
			collection.rects.resize(rect_count);

			for(int i = 0; i < rect_count; i++) {
				i32 l, t, r, b;
				sr.unpack(l, t, r, b);
				collection.rects[i] = IRect(l, t, r, b);
			}
		}

		for(int n = 0; n < collection_count; n++) {
			Collection &collection = collections[n];

			sr.seek(collection.offset);

			sr.signature(Str("<spranim_img>\0", 14));
			i16 type; sr >> type;

			if(type != '1' && type != '2')
				return ERROR("Unknown spranim_img type: %d", (int)type);
			collection.type = type;

			bool plain_type = type == '1';
			vector<char> data;
			int size = (n == collection_count - 1? sr.size() : collections[n + 1].offset) -
						collection.offset - 16;

			if(plain_type) {
				data.resize(size);
				sr.loadData(data);
			}
			else {
				i32 plain_size = 0;
				sr >> plain_size;
				EXPECT(zlibInflate(sr, data, size - 4));
				EXPECT(plain_size == data.size());
			}

			auto imgSr = memoryLoader(data);

			for(int l = 0; l < 4; l++) {
				i32 palSize; imgSr >> palSize;
				collection.palettes[l].resize(palSize);
				imgSr.loadData(span(&collection.palettes[l][0], palSize * 4));
				for(int i = 0; i < palSize; i++)
					collection.palettes[l][i] = swapBR(collection.palettes[l][i]);
			}

			int image_count = collection.frame_count * collection.dir_count * 4;
			collection.images.resize(image_count);
			collection.points.resize(image_count, int2(0, 0));
			
			for(int n = 0; n < image_count; n++) {
				DASSERT(imgSr.pos() < imgSr.size());
				char type; imgSr >> type;

				if(type == 1) {
					Palette palette;
					i32 x, y; imgSr.unpack(x, y);
					collection.points[n] = int2(x, y);
					collection.images[n] = EX_PASS(PackedTexture::legacyLoad(imgSr, palette));
				}
				else if(type == 0) { // empty image
				}
				else if(type == '<')
					break;
			}
		}

		for(int n = 0; n < collection_count; n++) {
			Collection &collection = collections[n];

			MultiPalette palette;
			for(int l = 0; l < Collection::layer_count; l++) {
				palette.offset[l] = (int)palette.colors.size();
				palette.colors.insert(palette.colors.end(),
						collection.palettes[l].begin(), collection.palettes[l].end());
			}
			for(int l = Collection::layer_count; l < layer_count; l++)
				palette.offset[l] = palette.offset[l - 1];

			int palette_index = (int)m_palettes.size();
			m_palettes.push_back(palette);

			for(int s = 0; s < (int)m_sequences.size(); s++) {
				if(seq2col[s] != n)
					continue;
				Sequence &seq = m_sequences[s];
				seq.dir_count = collection.dir_count;
				seq.palette_id = palette_index;

				for(int f = 0; f < seq.frame_count; f++) {
					Frame &frame = m_frames[seq.first_frame + f];
					if(frame.id >= 0) {
						if((frame.id >= collection.frame_count))
							frame.id = collection.frame_count - 1;
						frame.id = (int)m_images.size() + frame.id * seq.dir_count;
					}
				}
			}

			for(int f = 0; f < collection.frame_count; f++)
				for(int d = 0; d < collection.dir_count; d++) {
					int layer_inds[Collection::layer_count];
					//TODO: these indices might be wrong? add assertion
					collection.getLayerIndices(f, d, layer_inds);
					MultiImage image;

					image.rect = collection.rects[f * collection.dir_count + d];
					for(int l = 0; l < Collection::layer_count; l++) {
						image.images[l] = collection.images[layer_inds[l]];
						image.points[l] = collection.points[layer_inds[l]];
					}
						
					m_images.push_back(image);
				}
		}

		updateMaxRect();

		for(int s = 0; s < size(); s++) {
			Sequence &seq = m_sequences[s];
			string overlay_name = seq.name + "overlay";
			seq.overlay_id = findSequence(overlay_name.c_str());
			if(seq.overlay_id != -1 && m_sequences[seq.overlay_id].dir_count != seq.dir_count) {
				printf("Wrong dir_count in overlay: %s\n", overlay_name.c_str());
				seq.overlay_id = -1;
			}
		}

		return {};
	}

	template Ex<void> Sprite::legacyLoad(MemoryStream&, Str);
	template Ex<void> Sprite::legacyLoad(FileStream&, Str);
}
