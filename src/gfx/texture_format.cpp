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
#include <GL/gl.h>
#include <GL/glext.h>

namespace gfx
{
	namespace
	{
		struct FormatConv
		{
			FormatConv(int tBpp, TextureIdent a, int b, int c, int d, bool compr=0)
				:        bpp(tBpp), format(a), internal(b), gFormat(c), type(d), compressed(compr) { }

			int     	 bpp;                                                                                                                 //bytes
			TextureIdent format;
			int		     internal, gFormat, type;
			bool         compressed;
		};

		FormatConv fmtTab[] = {
			FormatConv(3, TI_R8G8B8,		GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE),
			FormatConv(4, TI_A8R8G8B8,		GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV),
			FormatConv(4, TI_X8R8G8B8,		GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV),
			FormatConv(2, TI_R5G6B5,		GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5),
			//	FormatConv(  2,	TI_X1R5G5B5,		0,0,0,0),
			FormatConv(2, TI_A1R5G5B5,		GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV),
			FormatConv(2, TI_A4R4G4B4,		GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV),
			FormatConv(1, TI_R3G3B2,		GL_R3_G3_B2, GL_RGB, GL_UNSIGNED_BYTE_3_3_2),
			FormatConv(1, TI_A8,			GL_ALPHA8, GL_ALPHA, GL_UNSIGNED_BYTE),
			//	FormatConv(  2,	TI_A8R3G3B2,		0,0,0,0),
			//	FormatConv(  2,	TI_X4R4G4B4,		0,0,0,0),
			FormatConv(4, TI_A2B10G10R10,	GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV),
			FormatConv(4, TI_A8B8G8R8,		GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV),
			//	FormatConv(  4,	TI_X8B8G8R8,		0,0,0,0),
			//	FormatConv(  4,	TI_G16R16,			0,0,0,0),
			FormatConv(4, TI_A2R10G10B10,	GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV),
			//	FormatConv(  8,	TI_A16B16G16R16,	0,0,0,0),

			FormatConv(1, TI_L8,			GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE),
			FormatConv(2, TI_A8L8,			GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE),
			//	FormatConv(  1,	TI_A4L4,			0,0,0,0),

			//	FormatConv(  2,	TI_V8U8,			0,0,0,0),
			//	FormatConv(  2,	TI_L6V5U5,			0,0,0,0),
			//	FormatConv(  4,	TI_X8L8V8U8,		0,0,0,0),
			//	FormatConv(  4,	TI_Q8W8V8U8,		0,0,0,0),
			//	FormatConv(  4,	TI_V16U16,			0,0,0,0),
			//	FormatConv(  4,	TI_A2W10V10U10,		0,0,0,0),

			//	FormatConv(  2,	TI_UYVY,			0,0,0,0),
			//	tFormatConv(  2,	TI_R8G8_B8G8,		0,0,0,0),
			//	FormatConv(  2,	TI_YUY2,			0,0,0,0),
			//	FormatConv(  2,	TI_G8R8_G8B8,		0,0,0,0),
			FormatConv(0, TI_DXT1,			GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0, 1),
			//	FormatConv(  1,	TI_DXT2,			0,0,0,0),
			FormatConv(1, TI_DXT3,			GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0, 1),
			//	FormatConv(  1,	TI_DXT4,			0,0,0,0),
			FormatConv(1, TI_DXT5,			GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0, 1),

			FormatConv(2, TI_L16,			GL_LUMINANCE16, GL_LUMINANCE, GL_UNSIGNED_SHORT),
			//	FormatConv(  8,	TI_Q16W16V16U16,	0,0,0,0),

			//	FormatConv(  2,	TI_R16F,			0,0,0,0),
			//	FormatConv(  4,	TI_G16R16F,			0,0,0,0),
			//	FormatConv(  8,	TI_A16B16G16R16F,	0,0,0,0),

			//	FormatConv(  4,	TI_R32F,			0,0,0,0),
			//	FormatConv(  8,	TI_G32R32F,			0,0,0,0),
			//	FormatConv(  16,TI_A32B32G32R32F,	0,0,0,0),

			FormatConv(0, TI_Unknown, 0, 0, 0, 0),
		};
	}

	TextureFormat::TextureFormat(int internal) {
		for(id = 0; fmtTab[id].format != TI_Unknown; id++)
			if(fmtTab[id].internal == internal)
				break;
	}

	TextureFormat::TextureFormat(int internal, int format, int type, bool compressed) {
		for(id = 0; fmtTab[id].format != TI_Unknown; id++)
			if(fmtTab[id].internal == internal && fmtTab[id].gFormat == format &&
			   fmtTab[id].type == type && fmtTab[id].compressed == compressed)
				break;
	}

	TextureFormat::TextureFormat() {
		for(id = 0; fmtTab[id].format != TI_Unknown; id++)
			;
	}

	TextureFormat::TextureFormat(TextureIdent fmt) {
		for(id = 0; fmtTab[id].format != TI_Unknown; id++)
			if(fmtTab[id].format == fmt)
				break;
	}

	TextureIdent TextureFormat::ident() const { return fmtTab[id].format; }
	int TextureFormat::glInternal() const { return fmtTab[id].internal; }
	int TextureFormat::glFormat() const { return fmtTab[id].gFormat; }
	int TextureFormat::glType() const { return fmtTab[id].type; }
	bool TextureFormat::isCompressed() const { return fmtTab[id].compressed; }
	int TextureFormat::bytesPerPixel() const { return fmtTab[id].bpp; }

	bool TextureFormat::operator==(const TextureFormat&t) const {
		return id == t.id;
	}

	bool TextureFormat::isSupported() const {
		if(fmtTab[id].format == TI_Unknown)
			return 0;

		if(fmtTab[id].compressed) {
			switch(fmtTab[id].format) {
			case TI_DXT1:
				return 0;                                                                                                                                                                                                                          //glExtAvaliable(OE_EXT_TEXTURE_COMPRESSION_S3TC)||glExtAvaliable(OE_EXT_TEXTURE_COMPRESSION_DXT1);

			case TI_DXT3:
			case TI_DXT5:
				return 0;                                                                                                                                                                                                                          //glExtAvaliable(OE_EXT_TEXTURE_COMPRESSION_S3TC);

			default:
				return 0;
			}
			//		return 0;
		}
		return 1;
	}

	int TextureFormat::evalImageSize(int width, int height) const {
		switch(fmtTab[id].format) {
		case TI_DXT1:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;

		case TI_DXT2:
		case TI_DXT3:
		case TI_DXT4:
		case TI_DXT5:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;

		default:
			return width * height * fmtTab[id].bpp;
		}
	}

	int TextureFormat::evalLineSize(int width) const {
		switch(fmtTab[id].format) {
		case TI_DXT1:
			return ((width + 3) / 4) * 8;

		case TI_DXT2:
		case TI_DXT3:
		case TI_DXT4:
		case TI_DXT5:
			return ((width + 3) / 4) * 16;

		default:
			return width * fmtTab[id].bpp;
		}
	}

}
