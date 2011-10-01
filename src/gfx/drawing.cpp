#include "gfx/device.h"
#include <GL/gl.h>

namespace gfx
{
	static float defaultMtx[16];

	void InitViewport(int2 size) {
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

		glGetFloatv(GL_MODELVIEW_MATRIX, defaultMtx);
	}

	void SetupIsometricView(int2 viewPos) {
		glLoadMatrixf(defaultMtx);
	}

	void Setup2DView() {
		glLoadMatrixf(defaultMtx);
	}

	void DrawQuad(int2 pos, int2 size, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(0.0f, 0.0f);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 1.0f);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(0.0f, 1.0f);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void DrawQuad(int2 pos, int2 size, float2 uv0, float2 uv1, Color color) {
		glBegin(GL_QUADS);

		glColor4ub(color.r, color.g, color.b, color.a);
		glTexCoord2f(uv0.x, uv0.y);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(uv1.x, uv0.y);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(uv1.x, uv1.y);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(uv0.x, uv1.y);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void DrawLine(int2 pos, int3 wp1, int3 wp2, Color color) {
		glBegin(GL_LINES);

		float2 p1 = WorldToScreen(wp1) + float2(pos);
		float2 p2 = WorldToScreen(wp2) + float2(pos);

		glColor4ub(color.r, color.g, color.b, color.a);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);

		glEnd();
	}

	void DrawBBox(int2 pos, int3 size) {
		float px = pos.x;
		float py = pos.y;

		float2 vx = WorldToScreen(int3(size.x, 0, 0));
		float2 vy = WorldToScreen(int3(0, size.y, 0));
		float2 vz = WorldToScreen(int3(0, 0, size.z));
		float2 fpos(pos);

		float2 pt[8] = {
			fpos + vx + vy,
			fpos + vx + vy + vz,
			fpos + vz + vy,
			fpos + vy,
			fpos + vx,
			fpos + vx + vz,
			fpos + vz,
			fpos + float2(0, 0),
		};

		int back[] = {6, 7, 3, 7, 4};
		int front[] = {5, 4, 0, 1, 2, 6, 5, 1, 0, 3, 2};

		glBegin(GL_LINE_STRIP);
		glColor3f(0.6f, 0.6f, 0.6f);
		for(int n = 0; n < COUNTOF(back); n++)
			glVertex2f(pt[back[n]].x, pt[back[n]].y);

		glColor3f(1.0f, 1.0f, 1.0f);
		for(int n = 0; n < COUNTOF(front); n++)
			glVertex2f(pt[front[n]].x, pt[front[n]].y);
		glEnd();
	}

	void Clear(Color color) {
		float4 col = color;
		glClearColor(col.x, col.y, col.z, col.w);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SetBlendingMode(BlendingMode mode) {
		if(mode == bmDisabled)
			glDisable(GL_BLEND);
		else
			glEnable(GL_BLEND);

		if(mode == bmNormal)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

}
