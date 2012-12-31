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
					collection.images[n].serializeZar(imgSr);
				}
				else if(type == 0) { // empty image
				}
				else if(type == '<')
					break;
			}
		}
	}

	Sprite::Sprite() :m_file_size(0) { }

	void Sprite::serialize(Serializer &sr) {
		if(sr.isLoading()) {
			loadFromSpr(sr);
			m_name = sr.name();
			m_file_size = sr.size();
		}
		else
			THROW("Saving not supported");
	}

	void Sprite::Collection::getLayerIndices(int frame_id, int dir_id, int *layer_indices) const {
		int image_count = m_frame_count * m_dir_count;

		for(int l = 0; l < 4; l++)
			layer_indices[l] = type == '1'?
				(frame_id * m_dir_count + dir_id) * 4 + l :
				dir_id * m_frame_count + l * image_count + frame_id;
	}

	Texture Sprite::Collection::getFrame(int frame_id, int dir_id) const {
		Texture out;

		int layer_indices[4];
		getLayerIndices(frame_id, dir_id, layer_indices);

		//TODO: there are still some bugs here
		int2 size(0, 0);
		for(int l = 0; l < COUNTOF(layer_indices); l++) {
			int id = layer_indices[l];
//			ASSERT(id >= 0 && id <= (int)images.size()); //TODO
			if(id >= (int)images.size())
				continue;

			size = max(size, points[id] + images[id].dimensions());
		}

		out.resize(size.x, size.y);
		memset(&out(0, 0), 0, size.x * size.y * 4);

		for(int l = 0; l < COUNTOF(layer_indices); l++) {
			int id = layer_indices[l];
//			ASSERT(id >= 0 && id <= (int)images.size()); //TODO
			if(id >= (int)images.size())
				continue;
			if(!images[id].width() || !images[id].height())
				continue;

			int2 lsize = images[id].dimensions();

			const Color *palette = &palettes[l][0];
			PalTexture tex;
			images[id].decompress(tex);
			const u8 *colors = &tex.m_colors[0];
			const u8 *alphas = &tex.m_alphas[0];
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

	bool Sprite::Collection::testPixel(const int2 &screen_pos, int frame_id, int dir_id) const {
		int layer_indices[4];
		getLayerIndices(frame_id, dir_id, layer_indices);
		for(int n = 0; n < COUNTOF(layer_indices); n++) {
			int id = layer_indices[n];
//			ASSERT(id >= 0 && id <= (int)images.size()); //TODO
			if(id >= (int)images.size())
				continue;

			const CompressedTexture &img = images[id];

			int2 pos = screen_pos - points[id];
			if(pos.x < 0 || pos.y < 0 || pos.x >= img.width() || pos.y >= img.height())
				continue;

			//TODO: write me
		//	if(img.alphas[pos.x + pos.y * img.width])
		//		return true;
		}
		return false;
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

	int Sprite::accessFrame(int seq_id, int frame_id, int dir_id) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());
		DASSERT(frame_id >= 0 && frame_id < (int)m_sequences[seq_id].frames.size());

		const Sequence &seq = m_sequences[seq_id];
		frame_id = seq.frames[frame_id].id;
		DASSERT(frame_id >= 0);
		
		const Collection &collection = m_collections[seq.collection_id];
		DASSERT(dir_id < collection.m_dir_count);

		return frame_id;
	}

	Texture Sprite::getFrame(int seq_id, int frame_id, int dir_id) const {
		frame_id = accessFrame(seq_id, frame_id, dir_id);
		const Collection &collection = m_collections[m_sequences[seq_id].collection_id];
		return collection.getFrame(frame_id, dir_id);
	}

	IRect Sprite::getRect(int seq_id, int frame_id, int dir_id) const {
		frame_id = accessFrame(seq_id, frame_id, dir_id);
		const Collection &collection = m_collections[m_sequences[seq_id].collection_id];
		return collection.rects[frame_id * collection.m_dir_count + dir_id] - m_offset;
	}

	IRect Sprite::getMaxRect() const {
		IRect out;
		bool is_set = false;

		for(int c = 0; c < (int)m_collections.size(); c++) {
			const Collection &collection = m_collections[c];
			for(int r = 0; r < (int)collection.rects.size(); r++) {
				const IRect &rect = collection.rects[r];
				out = is_set? sum(out, rect) : rect;
				is_set = true;
			}
		}

		return out - m_offset;
	}
		
	bool Sprite::testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const {
		frame_id = accessFrame(seq_id, frame_id, dir_id);
		const Collection &collection = m_collections[m_sequences[seq_id].collection_id];
		const IRect &rect = collection.rects[frame_id * collection.m_dir_count + dir_id];
		return rect.isInside(screen_pos + m_offset)?
			collection.testPixel(screen_pos + m_offset - rect.min, frame_id, dir_id) : false;
	}

	int Sprite::findSequence(const char *name) const {
		for(int n = 0; n < (int)m_sequences.size(); n++)
			if(m_sequences[n].name == name)
				return n;
		return -1;
	}
		
	int Sprite::findDir(int seq_id, float radians) const {
		DASSERT(seq_id >= 0 && seq_id < size());
//		DASSERT(radians >= 0.0f && radians < constant::pi * 2.0f);

		//TODO: wtf???
		float2 vec = angleToVector(radians);
		radians = vectorToAngle(-vec);
		
		int dir_count = dirCount(seq_id);
		float dir = radians * float(dir_count) * (0.5f / constant::pi) + 0.5f;
		return (int(dir) + dir_count) % dir_count;
	}

	void Sprite::printInfo() const {
		size_t bytes = sizeof(Sprite), img_bytes = 0;

		for(int n = 0; n < (int)m_sequences.size(); n++) {
			const Sequence &seq = m_sequences[n];
			bytes += sizeof(seq) + seq.name.size() + 1 + seq.frames.size() * sizeof(Frame);
		}
		for(int n = 0; n < (int)m_collections.size(); n++) {
			const Collection &col = m_collections[n];
			bytes += sizeof(col) + col.name.size() + 1 + col.rects.size() * sizeof(IRect) +
				col.images.size() * sizeof(PalTexture) + col.points.size() * sizeof(int2);
			for(int i = 0; i < COUNTOF(col.palettes); i++)
				bytes += col.palettes[i].size() * sizeof(Color);
			for(int i = 0; i < (int)col.images.size(); i++)
				img_bytes += col.images[i].memorySize();
		}

		printf("Sprite %s memory:\nstuff: %d KB\nimages: %d KB\nfile: %d KB\n",
				m_name.c_str(), (int)(bytes/1024), (int)(img_bytes/1024), m_file_size/1024);
	}

	void Sprite::printSequencesInfo() const {
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
