// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "base.h"

#include <fwk/gfx/gl_device.h>
#include <fwk/math/plane.h>
#include <fwk/math/rotation.h>
#include <fwk/sys/file_stream.h>

enum {
	// TODO: tune-able parameters?
	min0 = 16,
	min1 = 1024 * 4,
	min2 = 1024 * 1024,
	min3 = 1024 * 1024 * 128,

	max0 = 64,
	max1 = 16 * 1024,
	max2 = 1024 * 1024 * 4,
	max3 = 1024 * 1024 * 512,
};

void encodeInt(MemoryStream &sr, int value) {
	if(value >= -min0 && value < max0 - min0) {
		value += min0;
		sr.pack(u8(0x00 + value));
	} else if(value >= -min1 && value < max1 - min1) {
		value += min1;
		sr.pack(u8(0x40 + (value & 0x3f)), u8(value >> 6));
	} else if(value >= -min2 && value < max2 - min2) {
		value += min2;
		sr.pack(u8(0x80 + (value & 0x3f)), u8((value >> 6) & 0xff), u8((value >> 14)));
	} else if(value >= -min3 && value < max3 - min3) {
		value += min3;
		sr.pack(u8(0xc0 + (value & 0x1f)), u8((value >> 5) & 0xff), u8((value >> 13) & 0xff),
				u8(value >> 21));
	} else {
		sr.pack(u8(0xff), u8(value & 0xff), u8((value >> 8) & 0xff), u8((value >> 16) & 0xff),
				u8(value >> 24));
	}
}

int decodeInt(MemoryStream &sr) {
	u8 first_byte, bytes[4];
	sr >> first_byte;

	u8 header = first_byte & 0xc0;
	if(header == 0x00) {
		return (first_byte & 0x3f) - min0;
	} else if(header == 0x40) {
		sr.loadData(span(bytes, 1));
		return (i32(first_byte & 0x3f) | (i32(bytes[0]) << 6)) - min1;
	} else if(header == 0x80) {
		sr.loadData(span(bytes, 2));
		return (i32(first_byte & 0x3f) | (i32(bytes[0]) << 6) | (i32(bytes[1]) << 14)) - min2;
	} else {
		if(first_byte == 0xff) {
			sr.loadData(span(bytes, 4));
			return (i32(bytes[0]) | (i32(bytes[1]) << 8) | (i32(bytes[2]) << 16) |
					(i32(bytes[3]) << 24));
		} else {
			sr.loadData(span(bytes, 3));
			return (i32(first_byte & 0x1f) | i32(bytes[0] << 5) | (i32(bytes[1]) << 13) |
					(i32(bytes[2]) << 21)) -
				   min3;
		}
	}
}

void saveString(FileStream &sr, Str str) {
	if(str.size() < 255)
		sr << u8(str.size());
	else
		sr.pack((u8)255, str.size());
	sr.saveData(str);
}

Ex<string> loadString(FileStream &sr) {
	u32 len;
	u8 tmp;

	sr >> tmp;
	if(tmp < 255)
		len = tmp;
	else
		sr >> len;

	EXPECT(len <= sr.size() - sr.pos());
	string out(len, ' ');
	sr.loadData(span(&out[0], out.size()));
	return out;
}

float distance(const Box<float3> &a, const Box<float3> &b) {
	float3 p1 = vclamp(b.center(), a.min(), a.max());
	float3 p2 = vclamp(p1, b.min(), b.max());
	return distance(p1, p2);
}

float distanceSq(const FRect &a, const FRect &b) {
	float2 p1 = vclamp(b.center(), a.min(), a.max());
	float2 p2 = vclamp(p1, b.min(), b.max());
	return distanceSq(p1, p2);
}

bool areAdjacent(const IRect &a, const IRect &b) {
	if(b.x() < a.ex() && a.x() < b.ex())
		return a.ey() == b.y() || a.y() == b.ey();
	if(b.y() < a.ey() && a.y() < b.ey())
		return a.ex() == b.x() || a.x() == b.ex();
	return false;
}

bool areOverlapping(const IBox &a, const IBox &b) {
	for(int n = 0; n < 3; n++)
		if(b.min(n) >= a.max(n) || a.min(n) >= b.max(n))
			return false;
	return true;
}

bool areOverlapping(const FBox &a, const FBox &b) {
	// TODO: these epsilons shouldnt be here...
	for(int n = 0; n < 3; n++)
		if(b.min(n) >= a.max(n) - big_epsilon || a.min(n) >= b.max(n) - big_epsilon)
			return false;
	return true;
}

bool areOverlapping(const IRect &a, const IRect &b) {
	for(int n = 0; n < 2; n++)
		if(b.min(n) >= a.max(n) || a.min(n) >= b.max(n))
			return false;
	return true;
}
bool areOverlapping(const FRect &a, const FRect &b) {
	for(int n = 0; n < 2; n++)
		if(b.min(n) >= a.max(n) - big_epsilon || a.min(n) >= b.max(n) - big_epsilon)
			return false;
	return true;
}

MoveVector::MoveVector(const int2 &start, const int2 &end) {
	int2 diff = end - start;
	vec.x = diff.x < 0 ? -1 : diff.x > 0 ? 1 : 0;
	vec.y = diff.y < 0 ? -1 : diff.y > 0 ? 1 : 0;
	dx = fwk::abs(diff.x);
	dy = fwk::abs(diff.y);
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

float2 worldToScreen(const float3 &pos) {
	return float2(6.0f * (pos.x - pos.z), 3.0f * (pos.x + pos.z) - 7.0f * pos.y);
	//	7.0f * (pos.x + pos.z) + 6.0f * pos.y);
}

int2 worldToScreen(const int3 &pos) {
	return int2(6 * (pos.x - pos.z), 3 * (pos.x + pos.z) - 7 * pos.y);
	//	7 * (pos.x + pos.z) - 6 * pos.y);
}

float2 screenToWorld(const float2 &pos) {
	float x = pos.x * (1.0f / 12.0f);
	float y = pos.y * (1.0f / 6.0f);

	return float2(y + x, y - x);
}

int2 screenToWorld(const int2 &pos) {
	int x = pos.x / 12;
	int y = pos.y / 6;

	return int2(y + x, y - x);
}

Ray3F screenRay(const int2 &screen_pos) {
	float3 origin = asXZ(screenToWorld((float2)screen_pos));
	float3 dir = float3(-1.0f / 6.0f, -1.0f / 7.0f, -1.0f / 6.0f);
	return Ray3F(origin - dir * 2048.0f, dir / length(dir));
}

float3 project(const float3 &point, const Plane3F &plane) {
	float dist = dot(point, plane.normal()) - plane.distance0();
	return point - plane.normal() * dist;
}

vector<float3> genPointsOnPlane(const FBox &box, const float3 &dir, int density, bool outside) {
	DASSERT(density > 1);

	if(box.width() < big_epsilon && box.height() < big_epsilon && box.depth() < big_epsilon)
		return {box.center()};

	float radius = distance(box.center(), box.min());
	Plane3F plane(dir, box.center() + dir * radius);

	float3 origin = project(box.center(), plane);
	float3 other = project(box.min(), plane);
	if(distanceSq(other, origin) < big_epsilon) {
		for(auto corner : box.corners()) {
			other = project(corner, plane);
			if(distanceSq(other, origin) >= big_epsilon)
				break;
		}
	}

	if(distanceSq(other, origin) < big_epsilon)
		return {box.center()};

	float3 px = normalize(other - origin);
	float3 py = dir;
	float3 pz = cross(px, py);

	vector<float3> out;

	float mult = 1.0f / float(density - 1);
	for(int x = 0; x < density; x++)
		for(int z = 0; z < density; z++) {
			float3 point =
				origin + (px * (float(x) * mult - 0.5f) + pz * (float(z) * mult - 0.5f)) * radius;

			float isect = isectDist(Ray3F(point + dir, -dir), box);
			if(isect < inf)
				out.push_back(outside ? point : point - dir * (isect - 1.0f));
		}

	return out;
}

vector<float3> genPoints(const FBox &bbox, int density) {
	float3 offset = bbox.min();
	float3 mul = bbox.size() * (1.0f / (density - 1));
	vector<float3> out;

	// TODO: gen points on a plane, not inside a box
	for(int x = 0; x < density; x++)
		for(int y = 0; y < density; y++)
			for(int z = 0; z < density; z++)
				out.push_back(offset + float3(x * mul.x, y * mul.y, z * mul.z));

	return out;
}

void findPerpendicular(const float3 &v1, float3 &v2, float3 &v3) {
	DASSERT(lengthSq(v1) > big_epsilon);

	v2 = float3(-v1.y, v1.z, v1.x);
	v3 = cross(v1, v2);
}

float3 perturbVector(const float3 &v1, float rand1, float rand2, float strength) {
	float3 v2, v3;
	findPerpendicular(v1, v2, v3);

	float3 dir =
		normalize(float3(1.0f, (2.0f * rand1 - 1.0f) * strength, (2.0f * rand2 - 1.0f) * strength));
	return v1 * dir.x + v2 * dir.y + v3 * dir.z;
}

IntervalF IntervalF::operator*(const IntervalF &rhs) const {
	float a = min * rhs.min, b = min * rhs.max;
	float c = max * rhs.min, d = max * rhs.max;

	return IntervalF(fwk::min(fwk::min(a, b), fwk::min(c, d)),
					fwk::max(fwk::max(a, b), fwk::max(c, d)));
}

IntervalF IntervalF::operator*(float val) const {
	float tmin = min * val, tmax = max * val;
	return val < 0 ? IntervalF(tmax, tmin) : IntervalF(tmin, tmax);
}

IntervalF abs(const IntervalF &value) {
	if(value.min < 0.0f)
		return value.max < 0.0f ? IntervalF(-value.max, -value.min)
								: IntervalF(0.0f, max(-value.min, value.max));
	return value;
}

IntervalF floor(const IntervalF &value) { return IntervalF(floorf(value.min), floorf(value.max)); }

IntervalF min(const IntervalF &lhs, const IntervalF &rhs) {
	return IntervalF(min(lhs.min, rhs.min), min(lhs.max, rhs.max));
}

IntervalF max(const IntervalF &lhs, const IntervalF &rhs) {
	return IntervalF(max(lhs.min, rhs.min), max(lhs.max, rhs.max));
}

float intersection(const IntervalF idir[3], const IntervalF origin[3], const Box<float3> &box) {
	IntervalF l1, l2, lmin, lmax;

	l1 = idir[0] * (IntervalF(box.x()) - origin[0]);
	l2 = idir[0] * (IntervalF(box.ex()) - origin[0]);
	lmin = min(l1, l2);
	lmax = max(l1, l2);

	l1 = idir[1] * (IntervalF(box.y()) - origin[1]);
	l2 = idir[1] * (IntervalF(box.ey()) - origin[1]);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	l1 = idir[2] * (IntervalF(box.z()) - origin[2]);
	l2 = idir[2] * (IntervalF(box.ez()) - origin[2]);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	return lmin.min <= lmax.max ? lmin.min : (float)inf;
}

bool isInsideFrustum(const float3 &eye_pos, const float3 &eye_dir, float min_dot, const FBox &box) {
	for(auto corner : box.corners()) {
		float3 cvector = corner - eye_pos;
		float len = length(cvector);

		if(dot(cvector, eye_dir) >= min_dot * len)
			return true;
	}

	return false;
}

const Box<float3> rotateY(const Box<float3> &box, const float3 &origin, float angle) {
	auto corners = box.corners();
	float2 xz_origin = origin.xz();

	for(auto &corner : corners)
		corner = asXZY(rotateVector(corner.xz() - xz_origin, angle) + xz_origin, corner.y);
	return enclose(corners);
}

string32 toUTF32Checked(Str ref) {
	if(auto result = toUTF32(ref))
		return move(*result);
	FATAL("Error while converting string to UTF32");
}

string toUTF8Checked(const string32 &str) {
	if(auto result = toUTF8(str))
		return *result;
	FATAL("Error while converting string to UTF8");
}

void createWindow(const char *name, GlDevice &device, const int2 &res, const int2 &pos, bool fullscreen) {
	// TODO: date is refreshed only when game.o is being rebuilt
	auto title = format("FreeFT::%; built " __DATE__ " " __TIME__, name);
	auto flags = GlDeviceOpt::resizable | GlDeviceOpt::vsync;
	if(fullscreen)
		flags |= GlDeviceOpt::fullscreen;
	device.createWindow(title, res, flags, GlProfile::compatibility, 2.1);
	device.grabMouse(false);
}
