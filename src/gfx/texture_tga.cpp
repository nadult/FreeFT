#include "gfx/texture.h"

namespace gfx
{

	struct Header
	{
		u8  idlength;
		u8  colourmaptype;
		u8  datatypecode;
		u16 colourmaporigin;
		u16 colourmaplength;
		u8  colourmapdepth;
		u16 x_origin;
		u16 y_origin;
		u16 width;
		u16 height;
		u8  bitsperpixel;
		u8  imagedescriptor;
	} __attribute__((packed));

	void Texture::loadTGA(Serializer &sr) {
		Header hdr;
		enum { max_width = 2048 };

		sr.data(&hdr, sizeof(hdr));

		if(hdr.datatypecode != 2)
			THROW("Only uncompressed RGB data type is supported (id:%d)", (int)hdr.datatypecode);
		if(hdr.bitsperpixel != 24 && hdr.bitsperpixel != 32)
			THROW("Only 24 and 32-bit tga files are supported (bpp:%d)", (int)hdr.bitsperpixel);
		if(hdr.width > max_width)
				THROW("Bitmap is too wide (%d pixels): max width is %d", (int)hdr.width, (int)max_width);

		sr.seek(sr.pos() + hdr.idlength);

		unsigned bpp = hdr.bitsperpixel / 8;
		resize(hdr.width, hdr.height);

		if(bpp == 3) {
			for(int y = m_height - 1; y >= 0; y--) {
				char line[max_width * 3];
				sr.data(line, m_width * 3);
				Color *dst = this->line(y);
				for(int x = 0; x < m_width; x++)
					dst[x] = Color(line[x * 3 + 0], line[x * 3 + 1], line[x * 3 + 2]);
			}
		}
		else if(bpp == 4) {
			for(int y = m_height - 1; y >= 0; y--)
				sr.data(this->line(y), m_width * 4);
		}
	}

}
