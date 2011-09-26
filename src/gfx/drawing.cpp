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
