#include "gfx/texture.h"

namespace gfx
{

	void Texture::LoadBMP(Serializer &sr)
	{
		enum {
			maxWidth = 2048
		};

		{
			char sig[2];
			sr.Data(sig, 2);
			if(sig[0] != 'B' || sig[1] != 'M')
				ThrowException("Wrong BMP file signature");
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
				sr.Data(temp, 4 * 5);
				width  = hwidth;
				height = hheight;
				bpp    = hbpp;
				if(hSize > 40)
					sr.Seek(sr.Pos() + hSize - 40);

				if(compr != 0)
					ThrowException("Compressed bitmaps not supported");
			}

			if(bpp != 24 && bpp != 32 && bpp != 8)
				ThrowException(bpp, "-bit bitmaps are not supported (only 8, 24 and 32)");
			if(width > maxWidth)
				ThrowException("Bitmap is too wide (", width, " pixels): max width is ", maxWidth);
		}

		int bytesPerPixel = bpp / 8;
		int lineAlignment = 4 * ((bpp * width + 31) / 32) - bytesPerPixel * width;

		Resize(width, height);

		if(bytesPerPixel == 1) {
			Color palette[256];
			sr.Data(palette, sizeof(palette)); //TODO: check if palette is ok
			sr.Seek(offset);

			for(uint n = 0; n < COUNTOF(palette); n++)
				palette[n].a = 255;

			for(int y = height - 1; y >= 0; y--) {
				Color *dst = Line(y);
				u8 line[maxWidth];
				sr.Data(line, width);
				sr.Seek(sr.Pos() + lineAlignment);

				for(int x = 0; x < width; x++)
					dst[x] = palette[line[x]];
			}
		}
		else if(bytesPerPixel == 3) {
			sr.Seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				u8 line[maxWidth * 3];
				sr.Data(line, width * 3);

				Color *dst = Line(y);
				for(int x = 0; x < width; x++)
					dst[x] = Color(line[x * 3 + 0], line[x * 3 + 1], line[x * 3 + 2]); //TODO: check me
				sr.Seek(sr.Pos() + lineAlignment);
			}
		}
		else if(bytesPerPixel == 4) {
			sr.Seek(offset);
			for(int y = height - 1; y >= 0; y--) {
				sr.Data(Line(y), width * 4);
				sr.Seek(sr.Pos() + lineAlignment);
			}
		}
	}

}
