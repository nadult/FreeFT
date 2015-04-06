/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/drawing.h"
#include "fwk_opengl.h"

void drawLine(int3 wp1, int3 wp2, Color color) {
	glBegin(GL_LINES);

	float2 p1 = worldToScreen(wp1);
	float2 p2 = worldToScreen(wp2);

	glColor(color);
	glVertex2f(p1.x, p1.y);
	glVertex2f(p2.x, p2.y);

	glEnd();
}

void drawBBox(const FBox &box, Color col, bool is_filled) {
	float2 vx = worldToScreen(float3(box.width(), 0, 0));
	float2 vy = worldToScreen(float3(0, box.height(), 0));
	float2 vz = worldToScreen(float3(0, 0, box.depth()));
	float2 pos = worldToScreen(box.min);

	float2 pt[8] = {
		pos + vx + vy, pos + vx + vy + vz, pos + vz + vy, pos + vy,
		pos + vx,	  pos + vx + vz,	  pos + vz,	  pos,
	};

	bool is_flat = box.height() < constant::epsilon;

	if(is_filled) {
		static const int front[] = {0, 1, 2, 0, 2, 3, 0, 4, 5, 0, 5, 1, 1, 5, 6, 1, 6, 2};
		int count = is_flat ? 6 : arraySize(front);

		glBegin(GL_TRIANGLES);
		glColor(col);
		for(int n = 0; n < count; n++)
			glVertex2f(pt[front[n]].x, pt[front[n]].y);
		glEnd();
	} else {
		static const int back[] = {6, 7, 3, 7, 4};
		static const int front[] = {5, 4, 0, 1, 2, 6, 5, 1, 0, 3, 2};
		static const int front_flat[] = {0, 1, 2, 3, 0};

		glBegin(GL_LINE_STRIP);
		if(!is_flat) {
			glColor4ub(col.r >> 1, col.g >> 1, col.b >> 1, col.a / 2);
			for(size_t n = 0; n < arraySize(back); n++)
				glVertex2f(pt[back[n]].x, pt[back[n]].y);
		}

		glColor(col);
		if(is_flat)
			for(size_t n = 0; n < arraySize(front_flat); n++)
				glVertex2f(pt[front_flat[n]].x, pt[front_flat[n]].y);
		else
			for(size_t n = 0; n < arraySize(front); n++)
				glVertex2f(pt[front[n]].x, pt[front[n]].y);
		glEnd();
	}
}

void drawBBox(const IBox &wbox, Color col, bool is_filled) { drawBBox((FBox)wbox, col, is_filled); }
