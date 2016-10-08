/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/drawing.h"


void drawBBox(Renderer2D &out, const FBox &box, Color col, bool is_filled) {
	float2 vx = worldToScreen(float3(box.width(), 0, 0));
	float2 vy = worldToScreen(float3(0, box.height(), 0));
	float2 vz = worldToScreen(float3(0, 0, box.depth()));
	float2 pos = worldToScreen(box.min);

	float2 pt[8] = {
		pos + vx + vy, pos + vx + vy + vz, pos + vz + vy, pos + vy, pos + vx, pos + vx + vz,
		pos + vz, pos,
	};

	bool is_flat = box.height() < constant::epsilon;

	if(is_filled) {
		static const int front[] = {0, 1, 2, 0, 2, 3, 0, 4, 5, 0, 5, 1, 1, 5, 6, 1, 6, 2};
		float2 verts[arraySize(front)];
		int count = is_flat ? 6 : arraySize(front);
		for(int n = 0; n < count; n++)
			verts[n] = pt[front[n]];
		out.addTris(CRange<float2>(verts, verts + count), {}, {}, FColor(col));
	} else {
		static const int back[] = {7, 3, 7, 6, 7, 4};
		static const int front[] = {5, 4, 5, 6, 5, 1, 6, 2, 0, 4, 0, 1, 1, 2, 2, 3, 3, 0};
		static const int front_flat[] = {0, 1, 1, 2, 2, 3, 3, 0};
		Color col_back(col.r / 2, col.g / 2, col.b / 2, col.a / 2);

		if(!is_flat) {
			vector<float2> verts;
			for(auto id : back)
				verts.emplace_back(pt[id]);
			out.addLines(verts, {}, col_back);
		}

		vector<float2> verts;
		if(is_flat)
			for(auto id : front_flat)
				verts.emplace_back(pt[id]);
		else
			for(auto id : front)
				verts.emplace_back(pt[id]);
		out.addLines(verts, {}, col);
	}
}

void drawBBox(Renderer2D &out, const IBox &wbox, Color col, bool is_filled) {
	drawBBox(out, FBox(wbox), col, is_filled);
}

void drawLine(Renderer2D &out, int3 wp1, int3 wp2, Color color) {
	out.addLine(worldToScreen(wp1), worldToScreen(wp2), color);
}
