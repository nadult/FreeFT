/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "gfx/texture.h"
#include <png.h>

using namespace gfx;

namespace 
{

	struct Image
	{
		Image(): width(0), height(0), format(0) { }

		vector<u8> data;
		unsigned width, height, depth;
		int format;
	};

	struct PngInfo
	{
		PngInfo() : ptr(0), info(0) { }
		~PngInfo() {
			if(ptr || info)
				png_destroy_read_struct(&ptr, &info, 0);
		}

		png_structp ptr;
		png_infop info;
		int colorType, channels;
		int width, height;
	};

// TODO sprobowac zrobic bez statycznych zmiennych?
	vector<u8> fileData;
	int filePos;

	void png_read(png_structp, png_bytep data, png_size_t length)
	{
		int bytes = std::min(length, fileData.size() - filePos);

		memcpy(data, &fileData[filePos], bytes);
		filePos += bytes;
		return;
	}

	void png_error_func(png_structp png_ptr, png_const_charp message)
	{
		THROW("LibPNG Error: %s", message);
	}

	void png_warn_func(png_structp png_ptr, png_const_charp message)
	{
		return;
	}

	void InitPngInfo(PngInfo &png)
	{
		if(png_sig_cmp((png_byte *)&fileData[0], 0, 8))
			THROW("Wrong PNG file signature");

		png.ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, png_error_func, png_warn_func);
		if(!png.ptr)
			THROW("Out of memory");

		png.info = png_create_info_struct(png.ptr);
		if(!png.info)
			THROW("Out of memory");

		png_set_read_fn(png.ptr, 0, png_read);
		png_set_error_fn(png.ptr, 0, png_error_func, png_warn_func);
		png_read_info(png.ptr, png.info);
	}

	void ReadHeader(PngInfo &png, int *owidth, int *oheight, TextureIdent *oFormat)
	{
		int bitdepth;
		png_uint_32 width, height;

		png_get_IHDR(png.ptr, png.info, (png_uint_32 *)&width, (png_uint_32 *)&height,
					 &bitdepth, &png.colorType, 0, 0, 0);

		if(png.colorType == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
			png_set_expand_gray_1_2_4_to_8(png.ptr);
		if(png_get_valid(png.ptr, png.info, PNG_INFO_tRNS) && !(png_get_valid(png.ptr, png.info, PNG_INFO_PLTE)))
			png_set_tRNS_to_alpha(png.ptr);

		png_get_IHDR(png.ptr, png.info, (png_uint_32 *)&width, (png_uint_32 *)&height,
					 &bitdepth, &png.colorType, 0, 0, 0);

		if(bitdepth < 8) {
			bitdepth = 8;
			png_set_packing(png.ptr);
		}

#ifdef __LITTLE_ENDIAN__
		if(bitdepth == 16)
			png_set_swap(png.ptr);
#endif


		png_read_update_info(png.ptr, png.info);
		png.channels  = (int)png_get_channels(png.ptr, png.info);
		png.colorType = png_get_color_type(png.ptr, png.info);

		switch(png.colorType) {
		case PNG_COLOR_TYPE_PALETTE:
			*oFormat = png_get_valid(png.ptr, png.info, PNG_INFO_tRNS) ? TI_A8B8G8R8 : TI_R8G8B8;
			break;

		case PNG_COLOR_TYPE_GRAY:
			*oFormat = TI_R8G8B8;
			break;

		case PNG_COLOR_TYPE_GRAY_ALPHA:
			*oFormat = TI_A8B8G8R8;
			break;

		case PNG_COLOR_TYPE_RGB:
			*oFormat = TI_R8G8B8;
			break;

		case PNG_COLOR_TYPE_RGB_ALPHA:
			*oFormat = TI_A8B8G8R8;
			break;

		default:
			THROW("PNG color type not supported");
		}
		*owidth  = png.width = width;
		*oheight = png.height = height;
	}

	void ReadData(PngInfo &png, Color *out)
	{
		int width = png.width, height = png.height;

		vector<u8> temp;
		temp.resize(width * height * 4);
		vector <u8*> rowPointers(height);
			
		png_bytep trans = 0;
		int numTrans = 0;
			
		png_colorp palette = 0;
		int numPalette = 0;

		for(int y = 0; y < height; y++)
			rowPointers[y] = &temp[width * 4 * y];

		if(png.colorType == PNG_COLOR_TYPE_PALETTE) {
			if(png.channels != 1)
				THROW("Only 8-bit palettes are supported");


			if(!png_get_PLTE(png.ptr, png.info, &palette, &numPalette))
				THROW("Error while retrieving PNG palette");

			if(png_get_valid(png.ptr, png.info, PNG_INFO_tRNS))
				png_get_tRNS(png.ptr, png.info, &trans, &numTrans, 0);
		}

		png_read_image(png.ptr, &rowPointers[0]);

		for(int y = 0; y < height; y++) {
			Color *dst = out + width * y;
			u8 *src = rowPointers[y];

			if(png.colorType == PNG_COLOR_TYPE_PALETTE) {
				for(int x = 0; x < width; x++) {
					png_color &col = palette[src[x]];
					dst[x] = Color(col.red, col.green, col.blue, trans? trans[src[x]] : 255u);
				}
			}
			else if(png.colorType == PNG_COLOR_TYPE_GRAY) {
				for(int x = 0; x < width; x++)
					dst[x] = Color(src[x], src[x], src[x]);
			}
			else if(png.colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
				for(int x = 0; x < width; x++)
					dst[x] = Color(src[x * 2], src[x * 2], src[x * 2], src[x * 2 + 1]);
			}
			else if(png.colorType == PNG_COLOR_TYPE_RGB) {
				for(int x = 0; x < width; x++)
					dst[x] = Color(src[x * 3 + 0], src[x * 3 + 1], src[x * 3 + 2]);
			}
			else if(png.colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
				for(int x = 0; x < width; x++) //TODO: check
					dst[x] = Color(src[x * 4 + 0], src[x * 4 + 1], src[x * 4 + 2], src[x * 4 + 3]);
			}
		}
	}
}

namespace gfx
{

	void Texture::loadPNG(Serializer &sr) {
		Image image;

		try {
			PngInfo png;

			fileData.resize(sr.size());
			sr.data(&fileData[0], fileData.size());
			filePos = 0;

			InitPngInfo(png);
			int width, height;
			TextureIdent id;
			ReadHeader(png, &width, &height, &id);

			resize(width, height);
			ReadData(png, line(0));

			fileData.clear();
		} catch(...) {
			fileData.clear();
			throw;
		}
	}

}
