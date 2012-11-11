#include "gfx/texture.h"
#include <memory.h>

namespace gfx
{

	Texture::Texture(int w, int h) :data(w * h), m_width(w), m_height(h) { }

	Texture::Texture(Texture &&rhs) :m_width(rhs.m_width), m_height(rhs.m_height) {
		data.swap(rhs.data);
	}

	Texture::Texture() : m_width(0), m_height(0) { }

	void Texture::resize(int w, int h) {
		m_width  = w;
		m_height = h;
		data.resize(m_width * m_height);
	}
		
	void Texture::swap(Texture &tex) {
		std::swap(m_width, tex.m_width);
		std::swap(m_height, tex.m_height);
		data.swap(tex.data);
	}

	void Texture::free() {
		m_width = m_height = 0;
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
			if(ext == ".png") loadPNG(sr);
			else if(ext == ".bmp") loadBMP(sr);
			else if(ext == ".tga") loadTGA(sr);
			else if(ext == ".zar") loadZAR(sr);
			else THROW("%s format is not supported (Only BMP, TGA and PNG for now)", ext.c_str());
		}
	}

}
