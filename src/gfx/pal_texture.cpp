#include "gfx/texture.h"
#include "../libs/lz4/lz4.h"
#include "../libs/lz4/lz4hc.h"


namespace gfx {

	CompressedTexture::CompressedTexture()
		:m_width(0), m_height(0), m_palette_size(0), m_dec_stream_size(0) { }

	void CompressedTexture::decompress(PalTexture &out) const {
		out = PalTexture(m_width, m_height);

		if(m_dec_stream_size) {
			vector<char> stream(m_dec_stream_size);
			LZ4_uncompress(m_stream.data(), stream.data(), m_dec_stream_size);

			const u8 *ptr = (const u8*)stream.data();
			out.m_palette.resize(m_palette_size);

			const u8 *red = ptr, *green = red + m_palette_size, *blue = green + m_palette_size;

			for(int n = 0; n < m_palette_size; n++)
				out.m_palette[n] = Color(red[n], green[n], blue[n], 255u);
			ptr += m_palette_size * 3;

			int pixel_count = m_width * m_height;
			memcpy(out.m_colors.data(), ptr, pixel_count);
			memcpy(out.m_alphas.data(), ptr + pixel_count, pixel_count);
		}
	}

	void CompressedTexture::serializeZar(Serializer &sr) {
		PalTexture pal_texture;
		pal_texture.serializeZar(sr);
		pal_texture.compress(*this);
	}

	void CompressedTexture::serialize(Serializer &sr) {
		sr(m_width, m_height, m_palette_size, m_dec_stream_size);
		sr & m_stream;
	}

	PalTexture::PalTexture(int width, int height)
		:m_width(width), m_height(height) {
		m_colors.resize(m_width * m_height);
		m_alphas.resize(m_width * m_height);
	}

	void PalTexture::serializeZar(Serializer &sr) {
		sr.signature("<zar>", 6);

		char zar_type, dummy1, has_palette;

		sr(zar_type, dummy1, m_width, m_height, has_palette);
		ASSERT(m_width < 65536 && m_height < 65536);

		if(zar_type != 0x33 && zar_type != 0x34)
			THROW("Wrong zar type: %d", (int)zar_type);

		if(has_palette) {
			u32 pal_size; sr & pal_size;
			ASSERT(pal_size <= 256);
			m_palette.resize(pal_size);
			sr.data(m_palette.data(), pal_size * sizeof(Color));

			for(int n = 0; n < (int)m_palette.size(); n++)
				m_palette[n] = swapBR(m_palette[n]);
		}

		u8 default_col = 0;
		if((zar_type == 0x34 || zar_type == 0x33) && has_palette) {
			u32 def;
			sr & def;
			default_col = def & 255;
		}

		int end_pos = -1;
		if(!has_palette) {
			u32 rleSize; sr & rleSize;
			end_pos = (int)(sr.pos() + rleSize);
		}

		//TODO: unnecesary initialization
		int offset = 0, total_pixels = m_width * m_height;
		m_colors.resize(total_pixels);
		m_alphas.resize(total_pixels);

		while(offset < total_pixels) {
			u8 cmd; sr & cmd;
			int n_pixels = cmd >> 2;
			int command = cmd & 3;

			if(command == 0)
				memset(&m_alphas[offset], 0, n_pixels);
			else if(command == 1) {
				sr.data(&m_colors[offset], n_pixels);
				memset(&m_alphas[offset], 255, n_pixels);
			}
			else if(command == 2) {
				u8 buf[128];

				sr.data(buf, n_pixels * 2);
				for(int n = 0; n < n_pixels; n++) {
					m_colors[offset + n] = buf[n * 2 + 0];
					m_alphas[offset + n] = buf[n * 2 + 1];
				}
			}
			else {
				sr.data(&m_alphas[offset], n_pixels);
				memset(&m_colors[offset], default_col, n_pixels);
			}
			offset += n_pixels;
		}

		if(end_pos != -1)
			sr.seek(end_pos);
	}

	void PalTexture::compress(CompressedTexture &out) const {
		//TODO: palette reordering, pixel reordering
		int pixels = m_width * m_height;
		vector<u8> packed(pixels * 2 + m_palette.size() * 3);
		u8 *ptr = packed.data();

		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].r;
		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].g;
		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].b;
		
		memcpy(ptr, m_colors.data(), pixels); ptr += pixels;
		memcpy(ptr, m_alphas.data(), pixels); ptr += pixels;

		vector<char> stream(LZ4_compressBound(packed.size()));
		int compressed_size = LZ4_compressHC((char*)packed.data(), stream.data(), (int)packed.size());
		stream.resize(compressed_size);

		out.m_stream.resize(stream.size());
		memcpy(out.m_stream.data(), stream.data(), stream.size());

		out.m_dec_stream_size = (int)packed.size();
		out.m_width = m_width;
		out.m_height = m_height;
		out.m_palette_size = (int)m_palette.size();
	}

	void PalTexture::toBitmap(Bitmap &out) const {
		//TODO
	}
	
	void PalTexture::toTexture(Texture &out, const vector<Color> *palette) const {
		out.resize(m_width, m_height);
		Color *dst = out.line(0);
		int pixels = m_width * m_height;
		if(!palette)
			palette = &m_palette;
		int pal_size = (int)palette->size();
		const Color *pal = palette->data();

		for(int n = 0; n < pixels; n++) {
			DASSERT(m_colors[n] < pal_size); //TODO: maybe use ASSERT? this data is loaded from file...

			Color color = pal[m_colors[n]];
			color.a = m_alphas[n];
			dst[n] = color;
		}
	}

}
