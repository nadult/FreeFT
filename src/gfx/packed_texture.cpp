#include "gfx/texture.h"

namespace gfx {

	PackedTexture::PackedTexture()
		:m_width(0), m_height(0), m_colors_offset(0), m_alphas_offset(0), m_default_color(0) { }

	void PackedTexture::legacyLoad(Serializer &sr) {
		DASSERT(sr.isLoading());
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

			for(int n = 0; n < m_palette.size(); n++)
				m_palette[n] = swapBR(m_palette[n]);
		}

		u32 value;
		sr & value;

		m_default_color = 0;
		if((zar_type == 0x34 || zar_type == 0x33) && has_palette)
			m_default_color = value & 255; //TODO: endianess

		int offset = 0, total_pixels = m_width * m_height;
		vector<u8> cmds;
		vector<u8> colors;
		vector<u8> alphas;

		while(offset < total_pixels) {
			u8 buf[128];
			u8 cmd; sr & cmd;
			int n_pixels = cmd >> 2;
			int command = cmd & 3;
			cmds.push_back(cmd);

			if(command == 1) {
				sr.data(buf, n_pixels);
				colors.insert(colors.end(), buf, buf + n_pixels);
			}
			else if(command == 2) {
				sr.data(buf, n_pixels * 2);
				for(int n = 0; n < n_pixels; n++) {
					colors.push_back(buf[n * 2 + 0]);
					alphas.push_back(buf[n * 2 + 1]);
				}
			}
			else if(command == 3) {
				sr.data(buf, n_pixels);
				alphas.insert(alphas.end(), buf, buf + n_pixels);
			}
			offset += n_pixels;
		}

		m_data.resize(cmds.size() + colors.size() + alphas.size());
		memcpy(m_data.data(), cmds.data(), cmds.size());
		m_colors_offset = (int)cmds.size();
		memcpy(m_data.data() + m_colors_offset, colors.data(), colors.size());
		m_alphas_offset = m_colors_offset + (int)colors.size();
		memcpy(m_data.data() + m_alphas_offset, alphas.data(), alphas.size());
	}
		
	void PackedTexture::serialize(Serializer &sr) {
		sr(m_width, m_height, m_colors_offset, m_alphas_offset, m_default_color);
		sr & m_palette & m_data;
	}


	int PackedTexture::memorySize() const {
		return sizeof(PackedTexture) + m_data.size() + m_palette.size() * 4;
	}

	//TODO: speed up, etc.
	void PackedTexture::blit(Texture &out, const int2 &pos, const Color *__restrict pal) const {
		DASSERT(pos.x + m_width <= out.width() && pos.y + m_height <= out.height());
		DASSERT(pos.x >= 0 && pos.y >= 0);

		Color *dst = out.line(pos.y) + pos.x;
		int pixels = m_width * m_height, stride = out.width() - m_width;

		if(!pal)
			pal = m_palette.data();
		//TODO: checks
		const u8 *colors = m_data.data() + m_colors_offset;
		const u8 *alphas = m_data.data() + m_alphas_offset;
		const u8 *commands = m_data.data();

		for(int c = 0, line = m_width; c < m_colors_offset; c++) {
			int n_pixels = commands[c] >> 2;
			int command = commands[c] & 3;
			Color tcolors[64];

			if(command == 0)
				memset(tcolors, 0, n_pixels * sizeof(Color));
			else if(command == 1) {
				for(int i = 0; i < n_pixels; i++) {
					tcolors[i] = pal[colors[i]];
					tcolors[i].a = 255;
				}
				colors += n_pixels;
			}
			else if(command == 2) {
				for(int i = 0; i < n_pixels; i++) {
					Color color = pal[colors[i]];
					color.a = alphas[i];
					tcolors[i] = color;
				}
				colors += n_pixels;
				alphas += n_pixels;
			}
			else if(command == 3) {
				for(int i = 0; i < n_pixels; i++) {
					Color color = pal[m_default_color];
					color.a = alphas[i];
					tcolors[i] = color;
				}
				alphas += n_pixels;
			}

			Color *tsrc = tcolors;
			while(n_pixels) {
				int to_copy = min(n_pixels, line);
				for(int i = 0; i < to_copy; i++) if(tsrc[i].a) {
					if(dst[i].a) {
						float4 dstCol = dst[i];
						float4 srcCol = tsrc[i];
						float4 col = (srcCol - dstCol) * float(srcCol.w) + dstCol;
						dst[i] = Color(col.x, col.y, col.z, 1.0f);
					}
					else
						dst[i] = tsrc[i];
				}

				tsrc += to_copy;
				dst += to_copy;
				line -= to_copy;
				n_pixels -= to_copy;

				if(!line) {
					line = m_width;
					dst += stride;
				}
			}
		}
	}


	void PackedTexture::toTexture(Texture &out, const Palette *palette) const {
		out.resize(m_width, m_height);
		Color *dst = out.line(0);
		int pixels = m_width * m_height;
		if(!palette)
			palette = &m_palette;
		int pal_size = (int)palette->size();
		const Color *__restrict pal = palette->data();
		const u8 *colors = m_data.data() + m_colors_offset;
		const u8 *alphas = m_data.data() + m_alphas_offset;
		const u8 *commands = m_data.data();

		for(int c = 0; c < m_colors_offset; c++) {
			int n_pixels = commands[c] >> 2;
			int command = commands[c] & 3;

			if(command == 0)
				memset(dst, 0, n_pixels * sizeof(Color));
			else if(command == 1) {
				for(int i = 0; i < n_pixels; i++) {
					dst[i] = pal[colors[i]];
					dst[i].a = 255;
				}
				colors += n_pixels;
			}
			else if(command == 2) {
				for(int i = 0; i < n_pixels; i++) {
					Color color = pal[colors[i]];
					color.a = alphas[i];
					dst[i] = color;
				}
				colors += n_pixels;
				alphas += n_pixels;
			}
			else if(command == 3) {
				for(int i = 0; i < n_pixels; i++) {
					Color color = pal[m_default_color];
					color.a = alphas[i];
					dst[i] = color;
				}
				alphas += n_pixels;
			}

			dst += n_pixels;
		}
	}
		
	bool PackedTexture::testPixel(const int2 &pixel) const {
		if(pixel.x < 0 || pixel.y < 0 || pixel.x >= m_width || pixel.y >= m_height)
			return false;

		const u8 *alphas = m_data.data() + m_alphas_offset;
		const u8 *commands = m_data.data();

		int target_offset = pixel.x + pixel.y * m_width;

		for(int c = 0, offset = 0; c < m_colors_offset; c++) {
			int n_pixels = commands[c] >> 2;
			int command = commands[c] & 3;

			if(target_offset < offset + n_pixels)
				return command == 0? false : command == 1? true : alphas[target_offset - offset] != 0;

			if(command >= 2)
				alphas += n_pixels;
			offset += n_pixels;
		}

		return false;
	}

}


// OLD streaming code, not used
/*	
	struct Header {
		int width, height;
		int pal_size, color_off, alpha_off;
	};

	int PackedTexture::streamSize() const {
		return sizeof(Header) + m_palette.size() * 3 + m_data.size();
	}

	void PackedTexture::toStream(char *stream) const {
		u8 *ptr = (u8*)stream;

		Header header{m_width, m_height, (int)m_palette.size(), m_colors_offset, m_alphas_offset};
		memcpy(ptr, &header, sizeof(Header));
		ptr += sizeof(Header);

		//TODO: palette reordering, pixel reordering
		int pixels = m_width * m_height;

		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].r;
		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].g;
		for(int n = 0; n < (int)m_palette.size(); n++)
			*ptr++ = m_palette[n].b;

		memcpy(ptr, m_data.data(), m_data.size());
	}
	
	void PackedTexture::fromStream(const char *stream, int stream_size) {
		ASSERT(stream_size >= (int)sizeof(Header));
		const u8 *ptr = (const u8*)stream;

		Header header;
		memcpy(&header, ptr, sizeof(Header));
		ptr += sizeof(Header);

		m_width = header.width;
		m_height = header.height;

		int data_size = stream_size - sizeof(Header) - header.pal_size * 3;
		ASSERT(data_size >= 0);

		const u8 *red = ptr, *green = red + header.pal_size, *blue = green + header.pal_size;
		m_palette.resize(header.pal_size * 3);
		for(int n = 0; n < (int)m_palette.size(); n++)
			m_palette[n] = Color(red[n], green[n], blue[n]);

		ptr += header.pal_size * 3;
		m_data.resize(data_size);
		memcpy(m_data.data(), ptr, data_size);

		m_colors_offset = header.color_off;
		m_alphas_offset = header.alpha_off;
		ASSERT(m_colors_offset <= data_size && m_alphas_offset <= data_size);
	}*/
	

