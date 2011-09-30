#include "gfx/font.h"
#include "gfx/device.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <GL/gl.h>

namespace
{
	void Scan(std::fstream&f, char *buf, int bufSize)
	{
		for(int n = 0; n < bufSize; n++) {
			if(!f.good()) {
				buf[n] = 0;
				return;
			}

			f.get(buf[n]);
			if(buf[n] == ' ' || buf[n] == '\n' || buf[n] == '\t') {
				if(n == 0) {
					n--;
					continue;
				}
				buf[n] = 0;
				break;
			}
		}
		buf[bufSize - 1] = 0;
	}

}

namespace gfx
{
	Font::Font(const char *fontFileName)
	{
		std::fstream f(fontFileName, std::fstream::in);

		char  buf[1024];
		float scaleW = 0.0f, scaleH = 0.0f;
		int   num;

		while(true) {
			if(!f.good())
				ThrowException("Error while parsing font file: ", fontFileName);

			Scan(f, buf, 1024);
			if(strncmp(buf, "charset=", 8) == 0) {
				if(strcmp(buf + 8, "\"ANSI\"") != 0 && strcmp(buf + 8, "\"ASCII\""))
					ThrowException("Only ansi/ascii charsets are supported. Font: ", fontFileName);
			}
			else if(strncmp(buf, "scaleW=", 7) == 0)
				scaleW = atof(buf + 7);
			else if(strncmp(buf, "scaleH=", 7) == 0)
				scaleH = atof(buf + 7);
			else if(strncmp(buf, "pages=", 6) == 0) {
				if(atoi(buf + 6) != 1)
					ThrowException("Multi-page fonts not supported. Font: ", fontFileName);
			}
			else if(strcmp(buf, "chars") == 0)   {
				Scan(f, buf, 1024);
				if(strncmp(buf, "count=", 6) == 0) {
					num = atoi(buf + 6);
					break;
				}
			}
		}

		Assert(scaleW != 0.0f && scaleH != 0.0f);

		for(int n = 0; n < 256; n++) {
			chars[n * 2 + 0] = float2(0, 0);
			chars[n * 2 + 1] = float2(0, 0);
			offset[n]        = float2(0, 0);
			xadvance[n]      = 0.0f;
		}
		float advMid = 0.0f;

		for(int l = 0; l < num; l++) {
			while(true) {
				Scan(f, buf, 1024);
				if(!f.good())
					ThrowException("Error while parsing font file: ", fontFileName);

				if(strcmp(buf, "char") == 0)
					break;
			}

			Scan(f, buf, 1024);
			int id = atoi(buf + 3);
			InputAssert(id >= 0 && id <= 255);

			Scan(f, buf, 1024);
			int x = atoi(buf + 2);
			Scan(f, buf, 1024);
			int y = atoi(buf + 2);
			Scan(f, buf, 1024);
			int w = atoi(buf + 6);
			Scan(f, buf, 1024);
			int h = atoi(buf + 7);

			Scan(f, buf, 1024);
			int xoff = atoi(buf + 8);
			Scan(f, buf, 1024);
			int yoff = atoi(buf + 8);
			Scan(f, buf, 1024);
			int xadv = atoi(buf + 9);

			float2 pos  = float2(x / scaleW, y / scaleH);
			float2 size = float2(w / scaleW, h / scaleH);
			chars[id * 2 + 0] = pos;
			chars[id * 2 + 1] = pos + size;
			offset[id]        = float2(xoff / scaleW, yoff / scaleH);
			xadvance[id]      = float(xadv) / scaleW;
			advMid           += float(xadv);
		}

		xadvance[' '] = (advMid / float(num)) / scaleW;

		f.close();
		pos  = float2(0, 0);
		size = float2(24, 24);
	}

	size_t Font::GenQuads(const char *str, float2 *oPos, float2 *oUV, size_t bufSize) const
	{
		size_t len   = std::min(strlen(str), bufSize / 4);
		float  scale = 4.0f;
		float2 startPos = pos;

		for(size_t n = 0; n < len; n++) {
			u8 c = ((u8 *)str)[n];
			if(c == '\n') {
				pos = startPos + float2(0, size.y * 1.1f);
				startPos = pos;
			}

			const float2 &p1   = chars[c * 2 + 0], &p2 = chars[c * 2 + 1];
			float2       sp    = float2(pos) + float2(offset[c].x * size.x, offset[c].y * size.y) * scale;
			float2       tsize = float2((p2.x - p1.x) * size.x, (p2.y - p1.y) * size.y) * scale;

			float2 *p  = oPos + n * 4;
			float2 *uv = oUV + n * 4;

			p[0]  = sp;
			uv[0] = p1;
			p[1]  = sp + float2(tsize.x, 0);
			uv[1] = float2(p2.x, p1.y);
			p[2]  = sp + tsize;
			uv[2] = p2;
			p[3]  = sp + float2(0, tsize.y);
			uv[3] = float2(p1.x, p2.y);

			pos += float2(xadvance[c] * size.x * scale, 0);
		}

		return len;
	}

	void Font::Draw(const char *str) const {
		float2 tpos[1024], tuv[1024];
		int num = GenQuads(str, tpos, tuv, 1024);

		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		for(int n = 0; n < num; n++) {
			float2 *pptr = tpos + n * 4;
			float2 *uptr = tuv  + n * 4;
			
			glTexCoord2f(uptr[0].x, uptr[0].y); glVertex2f(pptr[0].x, pptr[0].y);
			glTexCoord2f(uptr[1].x, uptr[1].y); glVertex2f(pptr[1].x, pptr[1].y);
			glTexCoord2f(uptr[2].x, uptr[2].y); glVertex2f(pptr[2].x, pptr[2].y);
			glTexCoord2f(uptr[3].x, uptr[3].y); glVertex2f(pptr[3].x, pptr[3].y);
		}

		glEnd();
	}

}
