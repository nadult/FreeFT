#ifndef GFX_TEXTURE_H
#define GFX_TEXTURE_H

#include "base.h"

namespace gfx
{
	using namespace baselib;

	enum TextureIdent
	{
		TI_Unknown       =  0,

		TI_R8G8B8        = 20,
		TI_A8R8G8B8      = 21,
		TI_X8R8G8B8      = 22,
		TI_R5G6B5        = 23,
		TI_X1R5G5B5      = 24,
		TI_A1R5G5B5      = 25,
		TI_A4R4G4B4      = 26,
		TI_R3G3B2        = 27,
		TI_A8            = 28,
		TI_A8R3G3B2      = 29,
		TI_X4R4G4B4      = 30,
		TI_A2B10G10R10   = 31,
		TI_A8B8G8R8      = 32,
		TI_X8B8G8R8      = 33,
		TI_G16R16        = 34,
		TI_A2R10G10B10   = 35,
		TI_A16B16G16R16  = 36,

		TI_L8            = 50,
		TI_A8L8          = 51,
		TI_A4L4          = 52,

		TI_V8U8          = 60,
		TI_L6V5U5        = 61,
		TI_X8L8V8U8      = 62,
		TI_Q8W8V8U8      = 63,
		TI_V16U16        = 64,
		TI_A2W10V10U10   = 67,

		TI_UYVY          = 0x59565955,                                                                                   //MAKEFOURCC('U', 'Y', 'V', 'Y'),
		TI_R8G8_B8G8     = 0x47424752,                                                                                   //MAKEFOURCC('R', 'G', 'B', 'G'),
		TI_YUY2          = 0x32595559,                                                                                   //MAKEFOURCC('Y', 'U', 'Y', '2'),
		TI_G8R8_G8B8     = 0x42475247,                                                                                   //MAKEFOURCC('G', 'R', 'G', 'B'),
		TI_DXT1          = 0x31545844,                                                                                   //MAKEFOURCC('D', 'X', 'T', '1'),
		TI_DXT2          = 0x32545844,                                                                                   //MAKEFOURCC('D', 'X', 'T', '2'),
		TI_DXT3          = 0x33545844,                                                                                   //MAKEFOURCC('D', 'X', 'T', '3'),
		TI_DXT4          = 0x34545844,                                                                                   //MAKEFOURCC('D', 'X', 'T', '4'),
		TI_DXT5          = 0x35545844,                                                                                   //MAKEFOURCC('D', 'X', 'T', '5'),

		TI_L16           = 81,
		TI_Q16W16V16U16  =110,

		// Floating point surface formats
		// s10e5 formats (16-bits per channel)
		TI_R16F          = 111,
		TI_G16R16F       = 112,
		TI_A16B16G16R16F = 113,

		// IEEE s23e8 formats (32-bits per channel)
		TI_R32F          = 114,
		TI_G32R32F       = 115,
		TI_A32B32G32R32F = 116,

		TI_FORCE_DWORD   =0x7fffffff
	};

	class TextureFormat {
	public:
		TextureFormat(int internal, int format, int type, bool compressed = 0);
		TextureFormat(int internal);
		TextureFormat(TextureIdent fmt);
		TextureFormat();

		TextureIdent GetIdent() const;
		int GlInternal() const;
		int GlFormat() const;
		int GlType() const;
		bool IsCompressed() const;

		int BytesPerPixel() const;

		int EvalImageSize(int width, int height) const;
		int EvalLineSize(int width) const;

		bool operator==(const TextureFormat&) const;
		bool IsSupported() const;

	private:
		unsigned id;
	};

	class Texture: public RefCounter
	{
	public:
		Texture(int width, int height);
		Texture(Texture&&);
		Texture(const Texture&) = default;
		Texture();

		// poprzednie dane zostaja utracone
		void Resize(int width, int height);
		void Free();

		int Width() const { return width; }
		int Height() const { return height; }
		int2 Size() const { return int2(width, height); }
		bool IsEmpty() const { return data.empty(); }

		TextureFormat GetFormat() const { return TextureFormat(TI_A8B8G8R8); }

		// Loading from TGA, BMP, PNG, DDS
		void serialize(Serializer&);
		void Swap(Texture&);

		Color* Line(int y) { DASSERT(y < height); return &data[y * width]; }
		const Color* Line(int y) const { DASSERT(y < height); return &data[y * width]; }

		Color& operator()(int x, int y) { return data[x + y * width]; }
		const Color& operator()(int x, int y) const { return data[x + y * width]; }
		
		void LoadTGA(Serializer&);
		void LoadBMP(Serializer&);
		void LoadPNG(Serializer&);
		void LoadZAR(Serializer&);

private:
		vector<Color> data;
		int width, height;
	};
}
#endif
