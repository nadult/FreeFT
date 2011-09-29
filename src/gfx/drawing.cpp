#include "gfx/device.h"
#include <GL/gl.h>

namespace gfx
{

	void DrawQuad(int2 pos, int2 size) {
		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 0.0f);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(1.0f, 0.0f);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(1.0f, 1.0f);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(0.0f, 1.0f);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}

	void DrawQuad(int2 pos, int2 size, float2 uv0, float2 uv1) {
		glBegin(GL_QUADS);

		glTexCoord2f(uv0.x, uv0.y);		glVertex2i(pos.x, pos.y);
		glTexCoord2f(uv1.x, uv0.y);		glVertex2i(pos.x + size.x, pos.y);
		glTexCoord2f(uv1.x, uv1.y);		glVertex2i(pos.x + size.x, pos.y + size.y);
		glTexCoord2f(uv0.x, uv1.y);		glVertex2i(pos.x, pos.y + size.y);

		glEnd();
	}
	
	void DrawBBox(int2 pos, int3 size) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUADS);

		float px = pos.x;
		float py = pos.y;

		float2 vx = -WorldToScreen(int3(size.x, 0, 0));
		float2 vy =  WorldToScreen(int3(0, size.y, 0));
		float2 vz = -WorldToScreen(int3(0, 0, size.z));

		float2 pt[8] = {
			vx + vy,
			vx + vy + vz,
			vz + vy,
			vy,
			vx,
			vx + vz,
			vz,
			float2(0, 0),
		};

		int indices[5 * 4] = {
			1, 2, 6, 5,
			0, 1, 5, 4,
			
			0, 3, 7, 4,
			3, 2, 6, 7,
			1, 2, 3, 0,
		};

		glColor3f(0.8f, 0.8f, 0.8f);
		for(int n = 0; n < 4 * 2; n++)
			glVertex2f(px + pt[indices[n]].x, py + pt[indices[n]].y);
		
		glColor3f(1.0f, 1.0f, 1.0f);
		for(int n = 4 * 2; n < 4 * 5; n++)
			glVertex2f(px + pt[indices[n]].x, py + pt[indices[n]].y);

		glEnd();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
