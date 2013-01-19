/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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


	void Texture::saveTGA(Serializer &sr) {
		Header header;
		memset(&header, 0, sizeof(header));

		header.datatypecode = 2;
		header.colourmapdepth = 32;
		header.width = m_width;
		header.height = m_height;
		header.bitsperpixel = 32;
		header.imagedescriptor = 8;

		sr & header;
		vector<Color> line(m_width);
		for(int y = m_height - 1; y >= 0; y--) {
			memcpy(&line[0], this->line(y), m_width * sizeof(Color));
			for(int x = 0; x < m_width; x++)
				line[x] = Color(line[x].b, line[x].g, line[x].r, line[x].a);
			sr.data(&line[0], m_width * sizeof(Color));
		}
	}

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

		bool v_flip = hdr.imagedescriptor & 16;
		bool h_flip = hdr.imagedescriptor & 32;
		ASSERT(!v_flip && !h_flip && "TODO");

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
