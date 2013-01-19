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

#include "gfx/device.h"
#include <GL/gl.h>

namespace gfx
{
	
	void testGlError(const char*);

	static float s_default_matrix[16];
	static int2 s_viewport_size;

	void initViewport(int2 size) {
		s_viewport_size = size;
		glViewport(0, 0, size.x, size.y);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, size.x, 0, size.y, -1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0.0f, size.y, 0.0f);
		glScalef(1.0f, -1.0f, 1.0f);

		glGetFloatv(GL_MODELVIEW_MATRIX, s_default_matrix);
	}

	void lookAt(int2 pos) {
		glLoadMatrixf(s_default_matrix);
		glTranslatef(-pos.x, -pos.y, 0.0f);
	}

	void drawQuad(int2 pos, int2 size, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(0.0f, 0.0f);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 1.0f);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(0.0f, 1.0f);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void drawQuad(int2 pos, int2 size, float2 uv0, float2 uv1, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(uv0.x, uv0.y);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(uv1.x, uv0.y);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(uv1.x, uv1.y);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(uv0.x, uv1.y);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void drawLine(int2 p1, int2 p2, Color color) {
		glBegin(GL_LINES);

		glColor4ub(color.r, color.g, color.b, color.a);
		glVertex2i(p1.x, p1.y);
		glVertex2i(p2.x, p2.y);

		glEnd();
	}

	void drawLine(int3 wp1, int3 wp2, Color color) {
		glBegin(GL_LINES);

		float2 p1 = worldToScreen(wp1);
		float2 p2 = worldToScreen(wp2);

		glColor4ub(color.r, color.g, color.b, color.a);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);

		glEnd();
	}

	void drawRect(const IRect &rect, Color col) {
		glBegin(GL_LINE_STRIP);
		glColor4ub(col.r, col.g, col.b, col.a);

		glVertex2i(rect.min.x, rect.min.y);
		glVertex2i(rect.max.x, rect.min.y);
		glVertex2i(rect.max.x, rect.max.y);
		glVertex2i(rect.min.x, rect.max.y);
		glVertex2i(rect.min.x, rect.min.y);

		glEnd();
	}

	void drawBBox(const FBox &box, Color col, bool is_filled) {
		float2 vx = worldToScreen(float3(box.width(), 0, 0));
		float2 vy = worldToScreen(float3(0, box.height(), 0));
		float2 vz = worldToScreen(float3(0, 0, box.depth()));
		float2 pos = worldToScreen(box.min);

		float2 pt[8] = {
			pos + vx + vy,
			pos + vx + vy + vz,
			pos + vz + vy,
			pos + vy,
			pos + vx,
			pos + vx + vz,
			pos + vz,
			pos,
		};


		bool is_flat = box.height() < constant::epsilon;

		if(is_filled) {
			static const int front[] = {0, 1, 2, 0, 2, 3,   0, 4, 5, 0, 5, 1,   1, 5, 6, 1, 6, 2};
			int count = is_flat? 6 : COUNTOF(front);

			glBegin(GL_TRIANGLES);
			glColor4ub(col.r, col.g, col.b, col.a);
			for(int n = 0; n < count; n++)
				glVertex2f(pt[front[n]].x, pt[front[n]].y);
			glEnd();
		}
		else {
			static const int back[] = {6, 7, 3, 7, 4};
			static const int front[] = {5, 4, 0, 1, 2, 6, 5, 1, 0, 3, 2};
			static const int front_flat[] = {0, 1, 2, 3, 0};

			glBegin(GL_LINE_STRIP);
			if(!is_flat) {
				glColor4ub(col.r >> 1, col.g >> 1, col.b >> 1, col.a / 2);
				for(size_t n = 0; n < COUNTOF(back); n++)
					glVertex2f(pt[back[n]].x, pt[back[n]].y);
			}

			glColor4ub(col.r, col.g, col.b, col.a);
			if(is_flat) for(size_t n = 0; n < COUNTOF(front_flat); n++)
				glVertex2f(pt[front_flat[n]].x, pt[front_flat[n]].y);
			else for(size_t n = 0; n < COUNTOF(front); n++)
				glVertex2f(pt[front[n]].x, pt[front[n]].y);
			glEnd();
		}
	}
	
	void drawBBox(const IBox &wbox, Color col, bool is_filled) {
		drawBBox((FBox)wbox, col, is_filled);
	}

	void clear(Color color) {
		float4 col = color;
		glClearColor(col.x, col.y, col.z, col.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void setBlendingMode(BlendingMode mode) {
		if(mode == bmDisabled)
			glDisable(GL_BLEND);
		else
			glEnable(GL_BLEND);

		if(mode == bmNormal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void setScissorRect(const IRect &rect) {
		glScissor(rect.min.x, s_viewport_size.y - rect.max.y, rect.width(), rect.height());
		testGlError("glScissor");
	}

	void setScissorTest(bool is_enabled) {
		if(is_enabled)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

}

