#include "gfx/device.h"
#include "gfx/texture.h"
#include <GL/gl.h>

namespace gfx
{
	void TestGlError(const char *msg) {
		int err = glGetError();
		if(err == GL_NO_ERROR)
			return;

		switch(err) {
	#define CASE(e) case e: THROW("%s: " #e, msg);
			CASE(GL_INVALID_ENUM)
			CASE(GL_INVALID_VALUE)
			CASE(GL_INVALID_OPERATION)
			CASE(GL_STACK_OVERFLOW)
			CASE(GL_STACK_UNDERFLOW)
			CASE(GL_OUT_OF_MEMORY)
			default: THROW(msg);
	#undef CASE
		}
	}

	void SetTextureData(int level, TextureFormat fmt, int width, int height, const void *pixels) {
		DASSERT(width <= 4096 && height <= 4096);

		if(fmt.isCompressed()) {
			THROW("Texture compression is not supported");
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, level, fmt.glInternal(), width, height, 0,
							fmt.glFormat(), fmt.glType(), pixels);

			TestGlError("Error while loading texture surface to the device");
		}
	}

	void GetTextureData(int level, TextureFormat fmt, void *pixels) {
		if(fmt.isCompressed()) {
			THROW("Texture compression is not supported");
		}
		else {
			glGetTexImage(GL_TEXTURE_2D, level, fmt.glFormat(), fmt.glType(), pixels);
			TestGlError("Error while loading texture surface from the device");
		}
	}

	DTexture::DTexture() :m_id(0), m_mips(0) { }

	DTexture::~DTexture() {
		free();
	}

	void DTexture::free() {
		if(m_id > 0) {
			GLuint gl_id = m_id;
			glDeleteTextures(1, &gl_id);
			m_id = 0;
		}
	}
	
	void DTexture::create(int mips) {
		free();
		
		GLuint gl_id;
		glGenTextures(1, &gl_id);
		TestGlError("Error while creating texture");
		m_id = (int)gl_id;
		
		::glBindTexture(GL_TEXTURE_2D, gl_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mips > 1? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

		m_mips = mips;
	}

	void DTexture::setSurface(const Texture &in) {
		create(1);
		m_size = int2(in.width(), in.height());
		SetTextureData(0, in.format(), in.width(), in.height(), &in(0, 0));
	}

	void DTexture::getSurface(Texture &out) {
		out.resize(width(), height());
		TextureFormat fmt = out.format();
		GetTextureData(0, fmt, &out(0, 0));
	}

	void DTexture::serialize(Serializer &sr) {
		Texture surface;
		if(sr.isSaving())
			getSurface(surface);

		sr & surface;

		if(sr.isLoading())
			setSurface(surface);
	}

	void DTexture::bind() const {
		DASSERT(isValid());
		::glBindTexture(GL_TEXTURE_2D, m_id);
	}

	void DTexture::bind0() {
		::glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	void DTexture::createMip(int level, int w, int h, TextureFormat fmt) {
		DASSERT(isValid());

		GLint gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, m_id);
		
		glTexImage2D(GL_TEXTURE_2D, level, fmt.glInternal(), w, h, 0, fmt.glFormat(), fmt.glType(), 0);
					
		::glBindTexture(GL_TEXTURE_2D, gl_id);
	}
	
	void DTexture::updateMip(int mip, int x, int y, int w, int h, void *pix, int pixelsInRow) {
		DASSERT(isValid());

		GLint gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, m_id);
	
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pixelsInRow);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, pix);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					
		::glBindTexture(GL_TEXTURE_2D, gl_id);
	}

	TextureFormat DTexture::format() const {
		DASSERT(isValid());

		GLint internal = 0, gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, m_id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal);
		::glBindTexture(GL_TEXTURE_2D, gl_id);

		return TextureFormat(internal);
	}

	ResourceMgr<DTexture> DTexture::mgr("data/textures/", "");

}
