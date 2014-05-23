/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.*/

#ifndef GFX_OPENGL_H
#define GFX_OPENGL_H

#ifndef _WIN32
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GL/gl.h>
#include <GL/glext.h>
#include "gfx/device.h"
#include "base.h"

#ifdef _WIN32

#ifndef EXT_API
#define EXT_API extern
#endif

#define EXT_ENTRY __stdcall

extern "C" {

EXT_API void (EXT_ENTRY *glCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
EXT_API void (EXT_ENTRY *glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
EXT_API void (EXT_ENTRY *glCompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
EXT_API void (EXT_ENTRY *glCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
EXT_API void (EXT_ENTRY *glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
EXT_API void (EXT_ENTRY *glCompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);

EXT_API void (EXT_ENTRY *glBindBuffer)(GLenum target, GLuint buffer);
EXT_API void (EXT_ENTRY *glDeleteBuffers)(GLsizei n, const GLuint *buffers);
EXT_API void (EXT_ENTRY *glGenBuffers)(GLsizei n, GLuint *buffers);
EXT_API GLboolean (EXT_ENTRY *glIsBuffer)(GLuint buffer);
EXT_API void (EXT_ENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
EXT_API void (EXT_ENTRY *glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
EXT_API void (EXT_ENTRY *glGetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);
EXT_API GLvoid* (EXT_ENTRY *glMapBuffer)(GLenum target, GLenum access);
EXT_API GLboolean (EXT_ENTRY *glUnmapBuffer)(GLenum target);
EXT_API void (EXT_ENTRY *glGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
EXT_API void (EXT_ENTRY *glGetBufferPointerv)(GLenum target, GLenum pname, GLvoid* *params);

EXT_API void (EXT_ENTRY *glAttachShader)(GLuint program, GLuint shader);
EXT_API void (EXT_ENTRY *glBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
EXT_API void (EXT_ENTRY *glCompileShader)(GLuint shader);
EXT_API GLuint (EXT_ENTRY *glCreateProgram)(void);
EXT_API GLuint (EXT_ENTRY *glCreateShader)(GLenum type);
EXT_API void (EXT_ENTRY *glDeleteProgram)(GLuint program);
EXT_API void (EXT_ENTRY *glDeleteShader)(GLuint shader);
EXT_API void (EXT_ENTRY *glDetachShader)(GLuint program, GLuint shader);
EXT_API void (EXT_ENTRY *glDisableVertexAttribArray)(GLuint index);
EXT_API void (EXT_ENTRY *glEnableVertexAttribArray)(GLuint index);
EXT_API void (EXT_ENTRY *glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
EXT_API void (EXT_ENTRY *glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
EXT_API void (EXT_ENTRY *glGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *obj);
EXT_API GLint (EXT_ENTRY *glGetAttribLocation)(GLuint program, const GLchar *name);
EXT_API void (EXT_ENTRY *glGetProgramiv)(GLuint program, GLenum pname, GLint *params);
EXT_API void (EXT_ENTRY *glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
EXT_API void (EXT_ENTRY *glGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
EXT_API void (EXT_ENTRY *glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
EXT_API void (EXT_ENTRY *glGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
EXT_API GLint (EXT_ENTRY *glGetUniformLocation)(GLuint program, const GLchar *name);
EXT_API void (EXT_ENTRY *glGetUniformfv)(GLuint program, GLint location, GLfloat *params);
EXT_API void (EXT_ENTRY *glGetUniformiv)(GLuint program, GLint location, GLint *params);
EXT_API void (EXT_ENTRY *glGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
EXT_API void (EXT_ENTRY *glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
EXT_API void (EXT_ENTRY *glGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
EXT_API void (EXT_ENTRY *glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid* *pointer);

EXT_API void (EXT_ENTRY *glLinkProgram)(GLuint program);
EXT_API void (EXT_ENTRY *glShaderSource)(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length);
EXT_API void (EXT_ENTRY *glUseProgram)(GLuint program);

EXT_API void (EXT_ENTRY *glUniform1f)(GLint location, GLfloat v0);
EXT_API void (EXT_ENTRY *glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
EXT_API void (EXT_ENTRY *glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
EXT_API void (EXT_ENTRY *glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
EXT_API void (EXT_ENTRY *glUniform1i)(GLint location, GLint v0);
EXT_API void (EXT_ENTRY *glUniform2i)(GLint location, GLint v0, GLint v1);
EXT_API void (EXT_ENTRY *glUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
EXT_API void (EXT_ENTRY *glUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
EXT_API void (EXT_ENTRY *glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniform1iv)(GLint location, GLsizei count, const GLint *value);
EXT_API void (EXT_ENTRY *glUniform2iv)(GLint location, GLsizei count, const GLint *value);
EXT_API void (EXT_ENTRY *glUniform3iv)(GLint location, GLsizei count, const GLint *value);
EXT_API void (EXT_ENTRY *glUniform4iv)(GLint location, GLsizei count, const GLint *value);
EXT_API void (EXT_ENTRY *glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
EXT_API void (EXT_ENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

EXT_API void (EXT_ENTRY *glValidateProgram)(GLuint program);
EXT_API void (EXT_ENTRY *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);

}

#undef EXT_API
#undef EXT_ENTRY

#endif

#ifndef GFX_OPENGL_H_ONLY_EXTENSIONS
	
namespace gfx {
	
	void testGlError(const char*);

	DECLARE_ENUM(OpenglExtension,
		ext_texture_compression_dxt1,
		ext_texture_compression_s3tc,
		arb_texture_non_power_of_two
	);

	bool isExtensionSupported(OpenglExtension::Type);
	void glColor(Color);
	void glVertex(const float2&);
	void glTexCoord(const float2&);

}

#endif

#endif
