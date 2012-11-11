#include "gfx/texture.h"
#include <memory.h>

namespace gfx
{

	Texture::Texture(int w, int h) :data(w * h), width(w), height(h) { }

	Texture::Texture(Texture &&rhs) :width(rhs.width), height(rhs.height) {
		data.swap(rhs.data);
	}

	Texture::Texture() : width(0), height(0) { }

	void Texture::Resize(int w, int h) {
		width  = w;
		height = h;
		data.resize(width * height);
	}
		
	void Texture::Swap(Texture &tex) {
		std::swap(width, tex.width);
		std::swap(height, tex.height);
		data.swap(tex.data);
	}

	void Texture::Free() {
		width = height = 0;
		data.clear();
	}

	void Texture::serialize(Serializer &sr) {
		if(sr.isSaving())
			THROW("Saving textures not supported (yet)");

		string fileName = sr.name(), ext;
		{
			ext = fileName.substr(std::max(size_t(0), fileName.length() - 4));
			for(int n = 0, end = ext.length(); n < end; n++)
				ext[n] = tolower(ext[n]);
		}

		if(sr.isLoading()) {
			if(ext == ".png") LoadPNG(sr);
			else if(ext == ".bmp") LoadBMP(sr);
			else if(ext == ".tga") LoadTGA(sr);
			else if(ext == ".zar") LoadZAR(sr);
			else THROW("%s format is not supported (Only BMP, TGA and PNG for now)", ext.c_str());
		}
	}

}
