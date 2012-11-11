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

		if(fmt.IsCompressed()) {
			THROW("Texture compression is not supported");
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, level, fmt.GlInternal(), width, height, 0,
							fmt.GlFormat(), fmt.GlType(), pixels);

			TestGlError("Error while loading texture surface to the device");
		}
	}

	void GetTextureData(int level, TextureFormat fmt, void *pixels) {
		if(fmt.IsCompressed()) {
			THROW("Texture compression is not supported");
		}
		else {
			glGetTexImage(GL_TEXTURE_2D, level, fmt.GlFormat(), fmt.GlType(), pixels);
			TestGlError("Error while loading texture surface from the device");
		}
	}

	DTexture::DTexture() :id(0), mips(0) { }

	DTexture::~DTexture() {
		Free();
	}

	void DTexture::Free() {
		if(id > 0) {
			GLuint gl_id = id;
			glDeleteTextures(1, &gl_id);
			id = 0;
		}
	}
	
	void DTexture::Create(int tMips) {
		Free();
		
		GLuint gl_id;
		glGenTextures(1, &gl_id);
		TestGlError("Error while creating texture");
		id = (int)gl_id;
		
		::glBindTexture(GL_TEXTURE_2D, gl_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tMips > 1? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

		mips = tMips;
	}

	void DTexture::SetSurface(const Texture &in) {
		Create(1);
		size = int2(in.Width(), in.Height());
		SetTextureData(0, in.GetFormat(), in.Width(), in.Height(), &in(0, 0));
	}

	void DTexture::GetSurface(Texture &out) {
		out.Resize(Width(), Height());
		TextureFormat fmt = out.GetFormat();
		GetTextureData(0, fmt, &out(0, 0));
	}

	void DTexture::serialize(Serializer &sr) {
		Texture surface;
		if(sr.isSaving())
			GetSurface(surface);

		sr & surface;

		if(sr.isLoading())
			SetSurface(surface);
	}

	void DTexture::Bind() const {
		DASSERT(IsValid());
		::glBindTexture(GL_TEXTURE_2D, id);
	}

	void DTexture::Bind0() {
		::glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	void DTexture::CreateMip(int level, int w, int h, TextureFormat fmt) {
		DASSERT(IsValid());

		GLint gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, id);
		
		glTexImage2D(GL_TEXTURE_2D, level, fmt.GlInternal(), w, h, 0, fmt.GlFormat(), fmt.GlType(), 0);
					
		::glBindTexture(GL_TEXTURE_2D, gl_id);
	}
	
	void DTexture::UpdateMip(int mip, int x, int y, int w, int h, void *pix, int pixelsInRow) {
		DASSERT(IsValid());

		GLint gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, id);
	
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pixelsInRow);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, pix);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					
		::glBindTexture(GL_TEXTURE_2D, gl_id);
	}

	TextureFormat DTexture::GetFormat() const {
		DASSERT(IsValid());

		GLint internal = 0, gl_id;
		glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &gl_id);
		::glBindTexture(GL_TEXTURE_2D, id);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal);
		::glBindTexture(GL_TEXTURE_2D, gl_id);

		return TextureFormat(internal);
	}

	ResourceMgr<DTexture> DTexture::mgr("data/textures/", "");

}
