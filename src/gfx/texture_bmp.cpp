/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "gfx/texture.h"

namespace gfx
{

	void Texture::loadBMP(Serializer &sr)
	{
		enum {
			maxwidth = 2048
		};

		{
			char sig[2];
			sr.data(sig, 2);
			if(sig[0] != 'B' || sig[1] != 'M')
				THROW("Wrong BMP file signature");
		}

		int offset;
		{
			i32 size, reserved, toffset;
			sr& size & reserved & toffset;
			offset = toffset;
		}
		int width, height, bpp;
		{
			i32 hSize;
			sr& hSize;
			if(hSize == 12) {
				u16 hwidth, hheight, planes, hbpp;
				sr& hwidth&hheight&planes&hbpp;

				width  = hwidth;
				height = hheight;
				bpp    = hbpp;
			}
			else {
				i32 hwidth, hheight;
				i16 planes, hbpp;
				i32 compr, temp[5];

				sr&hwidth&hheight&planes&hbpp;
				sr&compr;
				sr.data(temp, 4 * 5);
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
			sr.data(palette, sizeof(palette)); //TODO: check if palette is ok
			sr.seek(offset);

			for(uint n = 0; n < COUNTOF(palette); n++)
				palette[n].a = 255;

			for(int y = height - 1; y >= 0; y--) {
				Color *dst = this->line(y);
				u8 line[maxwidth];
				sr.data(line, width);
				sr.seek(sr.pos() + lineAlignment);

				for(int x = 0; x < width; x++)
					dst[x] = palette[line[x]];
			}
		}
		else if(bytesPerPixel == 3) {
			sr.seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				u8 line[maxwidth * 3];
				sr.data(line, width * 3);

				Color *dst = this->line(y);
				for(int x = 0; x < width; x++)
					dst[x] = Color(line[x * 3 + 0], line[x * 3 + 1], line[x * 3 + 2]); //TODO: check me
				sr.seek(sr.pos() + lineAlignment);
			}
		}
		else if(bytesPerPixel == 4) {
			sr.seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				sr.data(this->line(y), width * 4);
				sr.seek(sr.pos() + lineAlignment);
			}
		}
	}

}
