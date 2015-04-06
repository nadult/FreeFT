/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "base.h"
#include <cstdlib>

MoveVector::MoveVector(const int2 &start, const int2 &end) {
	int2 diff = end - start;
	vec.x = diff.x < 0 ? -1 : diff.x > 0 ? 1 : 0;
	vec.y = diff.y < 0 ? -1 : diff.y > 0 ? 1 : 0;
	dx = abs(diff.x);
	dy = abs(diff.y);
	ddiag = min(dx, dy);
	dx -= ddiag;
	dy -= ddiag;
}
MoveVector::MoveVector() : vec(0, 0), dx(0), dy(0), ddiag(0) {}

/* World To Screen Matrix:
 * 			    | 6  3  7|
 * 		*	    | 0 -7  6|
 * 			    |-6  3  7|
 *
 * [WX WY WZ]   [SX SY SZ]
 *
 */

const float2 worldToScreen(const float3 &pos) {
	return float2(6.0f * (pos.x - pos.z), 3.0f * (pos.x + pos.z) - 7.0f * pos.y);
	//	7.0f * (pos.x + pos.z) + 6.0f * pos.y);
}

const int2 worldToScreen(const int3 &pos) {
	return int2(6 * (pos.x - pos.z), 3 * (pos.x + pos.z) - 7 * pos.y);
	//	7 * (pos.x + pos.z) - 6 * pos.y);
}

const float2 screenToWorld(const float2 &pos) {
	float x = pos.x * (1.0f / 12.0f);
	float y = pos.y * (1.0f / 6.0f);

	return float2(y + x, y - x);
}

const int2 screenToWorld(const int2 &pos) {
	int x = pos.x / 12;
	int y = pos.y / 6;

	return int2(y + x, y - x);
}

const Ray screenRay(const int2 &screen_pos) {
	float3 origin = asXZ(screenToWorld((float2)screen_pos));
	float3 dir = float3(-1.0f / 6.0f, -1.0f / 7.0f, -1.0f / 6.0f);
	return Ray(origin, dir / length(dir));
}

vector<float3> genPointsOnPlane(const FBox &box, const float3 &dir, int density, bool outside) {
	if(box.width() < constant::epsilon && box.height() < constant::epsilon &&
	   box.depth() < constant::epsilon)
		return {box.center()};

	float radius = distance(box.center(), box.min);
	Plane plane(box.center() + dir * radius, dir);

	float3 origin = project(box.center(), plane);
	float3 other = project(box.min, plane);
	if(distanceSq(other, origin) < constant::epsilon) {
		float3 corners[8];
		box.getCorners(corners);
		for(int n = 0; n < arraySize(corners) && distanceSq(other, origin) < constant::epsilon; n++)
			other = project(corners[n], plane);
	}

	if(distanceSq(other, origin) < constant::epsilon)
		return {box.center()};

	float3 plane_x = normalized(other - origin);
	float3 plane_y = dir;
	float3 plane_z = cross(plane_x, plane_y);

	vector<float3> out;

	float mult = 1.0f / float(density - 1);
	for(int x = 0; x < density; x++)
		for(int z = 0; z < density; z++) {
			float3 point =
				origin +
				(plane_x * (float(x) * mult - 0.5f) + plane_z * (float(z) * mult - 0.5f)) * radius;

			float isect = intersection(Ray(point + dir, -dir), box);
			if(isect < constant::inf)
				out.push_back(outside ? point : point - dir * (isect - 1.0f));
		}

	return std::move(out);
}

vector<float3> genPoints(const FBox &bbox, int density) {
	float3 offset = bbox.min;
	float3 mul = bbox.size() * (1.0f / (density - 1));
	vector<float3> out;

	// TODO: gen points on a plane, not inside a box
	for(int x = 0; x < density; x++)
		for(int y = 0; y < density; y++)
			for(int z = 0; z < density; z++)
				out.push_back(offset + float3(x * mul.x, y * mul.y, z * mul.z));

	return std::move(out);
}

void findPerpendicular(const float3 &v1, float3 &v2, float3 &v3) {
	DASSERT(lengthSq(v1) > constant::epsilon);

	v2 = float3(-v1.y, v1.z, v1.x);
	v3 = cross(v1, v2);
}

float3 perturbVector(const float3 &v1, float rand1, float rand2, float strength) {
	float3 v2, v3;
	findPerpendicular(v1, v2, v3);

	float3 dir = normalized(
		float3(1.0f, (2.0f * rand1 - 1.0f) * strength, (2.0f * rand2 - 1.0f) * strength));
	return v1 * dir.x + v2 * dir.y + v3 * dir.z;
}
