/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/texture.h"

namespace gfx
{

	void Texture::loadBMP(Stream &sr)
	{
		enum {
			maxwidth = 2048
		};

		{
			char sig[2];
			sr >> sig;
			if(sig[0] != 'B' || sig[1] != 'M')
				THROW("Wrong BMP file signature");
		}

		int offset;
		{
			i32 size, reserved, toffset;
			sr.unpack(size, reserved, toffset);
			offset = toffset;
		}
		int width, height, bpp;
		{
			i32 hSize;
			sr >> hSize;
			if(hSize == 12) {
				u16 hwidth, hheight, planes, hbpp;
				sr.unpack(hwidth, hheight, planes, hbpp);

				width  = hwidth;
				height = hheight;
				bpp    = hbpp;
			}
			else {
				i32 hwidth, hheight;
				i16 planes, hbpp;
				i32 compr, temp[5];

				sr.unpack(hwidth, hheight, planes, hbpp);
				sr >> compr;
				sr.loadData(temp, 4 * 5);
				width  = hwidth;
				height = hheight;
				bpp    = hbpp;
				if(hSize > 40)
					sr.seek(sr.pos() + hSize - 40);

				if(compr != 0)
					THROW("Compressed bitmaps not supported");
			}

			if(bpp != 24 && bpp != 32 && bpp != 8)
				THROW("%d-bit bitmaps are not supported (only 8, 24 and 32)", bpp);
			if(width > maxwidth)
				THROW("Bitmap is too wide (%d pixels): max width is %d",width, (int)maxwidth);
		}

		int bytesPerPixel = bpp / 8;
		int lineAlignment = 4 * ((bpp * width + 31) / 32) - bytesPerPixel * width;

		resize(width, height);

		if(bytesPerPixel == 1) {
			Color palette[256];
			sr.loadData(palette, sizeof(palette)); //TODO: check if palette is ok
			sr.seek(offset);

			for(uint n = 0; n < arraySize(palette); n++)
				palette[n].a = 255;

			for(int y = height - 1; y >= 0; y--) {
				Color *dst = this->line(y);
				u8 line[maxwidth];
				sr.loadData(line, width);
				sr.seek(sr.pos() + lineAlignment);

				for(int x = 0; x < width; x++)
					dst[x] = palette[line[x]];
			}
		}
		else if(bytesPerPixel == 3) {
			sr.seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				u8 line[maxwidth * 3];
				sr.loadData(line, width * 3);

				Color *dst = this->line(y);
				for(int x = 0; x < width; x++)
					dst[x] = Color(line[x * 3 + 0], line[x * 3 + 1], line[x * 3 + 2]); //TODO: check me
				sr.seek(sr.pos() + lineAlignment);
			}
		}
		else if(bytesPerPixel == 4) {
			sr.seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				sr.loadData(this->line(y), width * 4);
				sr.seek(sr.pos() + lineAlignment);
			}
		}
	}

}
