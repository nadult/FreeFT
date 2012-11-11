#include "gfx/texture.h"

namespace gfx
{

	struct Header
	{
		u8  idLength;
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

	void Texture::LoadTGA(Serializer &sr) {
		Header hdr;
		enum { maxWidth = 2048 };

		sr.data(&hdr, sizeof(hdr));

		if(hdr.datatypecode != 2)
			THROW("Only uncompressed RGB data type is supported (id:%d)", (int)hdr.datatypecode);
		if(hdr.bitsperpixel != 24 && hdr.bitsperpixel != 32)
			THROW("Only 24 and 32-bit tga files are supported (bpp:%d)", (int)hdr.bitsperpixel);
		if(hdr.width > maxWidth)
				THROW("Bitmap is too wide (%d pixels): max width is %d", (int)hdr.width, (int)maxWidth);

		sr.seek(sr.pos() + hdr.idLength);

		unsigned bpp = hdr.bitsperpixel / 8;
		Resize(hdr.width, hdr.height);

		if(bpp == 3) {
			for(int y = height - 1; y >= 0; y--) {
				char line[maxWidth * 3];
				sr.data(line, width * 3);
				Color *dst = Line(y);
				for(int x = 0; x < width; x++)
					dst[x] = Color(line[x * 3 + 0], line[x * 3 + 1], line[x * 3 + 2]);
			}
		}
		else if(bpp == 4) {
			for(int y = height - 1; y >= 0; y--)
				sr.data(Line(y), width * 4);
		}
	}

}
