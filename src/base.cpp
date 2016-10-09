/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "base.h"

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

void encodeInt(Stream &sr, int value) {
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

int decodeInt(Stream &sr) {
	u8 first_byte, bytes[4];
	sr.loadData(&first_byte, 1);

	u8 header = first_byte & 0xc0;
	if(header == 0x00) {
		return (first_byte & 0x3f) - min0;
	} else if(header == 0x40) {
		sr.loadData(bytes, 1);
		return (i32(first_byte & 0x3f) | (i32(bytes[0]) << 6)) - min1;
	} else if(header == 0x80) {
		sr.loadData(bytes, 2);
		return (i32(first_byte & 0x3f) | (i32(bytes[0]) << 6) | (i32(bytes[1]) << 14)) - min2;
	} else {
		if(first_byte == 0xff) {
			sr.loadData(bytes, 4);
			return (i32(bytes[0]) | (i32(bytes[1]) << 8) | (i32(bytes[2]) << 16) |
					(i32(bytes[3]) << 24));
		} else {
			sr.loadData(bytes, 3);
			return (i32(first_byte & 0x1f) | i32(bytes[0] << 5) | (i32(bytes[1]) << 13) |
					(i32(bytes[2]) << 21)) -
				   min3;
		}
	}
}

uint toFlags(const char *input, CRange<const char *> strings, uint first_flag) {
	const char *iptr = input;

	uint out_value = 0;
	while(*iptr) {
		const char *next_space = strchr(iptr, ' ');
		int len = next_space ? next_space - iptr : strlen(iptr);

		bool found = false;
		for(int e = 0; e < strings.size(); e++)
			if(strncmp(iptr, strings[e], len) == 0 && strings[e][len] == 0) {
				out_value |= first_flag << e;
				found = true;
				break;
			}

		if(!found) {
			char flags[1024], *ptr = flags;
			for(int i = 0; i < strings.size(); i++)
				ptr += snprintf(ptr, sizeof(flags) - (ptr - flags), "%s ", strings[i]);
			if(strings.size())
				ptr[-1] = 0;

			THROW("Error while converting string \"%s\" to flags (%s)", input, flags);
		}

		if(!next_space)
			break;
		iptr = next_space + 1;
	}

	return out_value;
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
	return Ray(origin - dir * 1024.0f, dir / length(dir));
}

const float3 project(const float3 &point, const Plane &plane) {
	float dist = dot(point, plane.normal()) - plane.distance();
	return point - plane.normal() * dist;
}

vector<float3> genPointsOnPlane(const FBox &box, const float3 &dir, int density, bool outside) {
	DASSERT(density > 1);

	if(box.width() < constant::epsilon && box.height() < constant::epsilon &&
	   box.depth() < constant::epsilon)
		return {box.center()};

	float radius = distance(box.center(), box.min);
	Plane plane(dir, box.center() + dir * radius);

	float3 origin = project(box.center(), plane);
	float3 other = project(box.min, plane);
	if(distanceSq(other, origin) < constant::epsilon) {
		for(auto corner : box.corners()) {
			other = project(corner, plane);
			if(distanceSq(other, origin) >= constant::epsilon)
				break;
		}
	}

	if(distanceSq(other, origin) < constant::epsilon)
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

	float3 dir =
		normalize(float3(1.0f, (2.0f * rand1 - 1.0f) * strength, (2.0f * rand2 - 1.0f) * strength));
	return v1 * dir.x + v2 * dir.y + v3 * dir.z;
}

Interval Interval::operator*(const Interval &rhs) const {
	float a = min * rhs.min, b = min * rhs.max;
	float c = max * rhs.min, d = max * rhs.max;

	return Interval(fwk::min(fwk::min(a, b), fwk::min(c, d)),
					fwk::max(fwk::max(a, b), fwk::max(c, d)));
}

Interval Interval::operator*(float val) const {
	float tmin = min * val, tmax = max * val;
	return val < 0 ? Interval(tmax, tmin) : Interval(tmin, tmax);
}

Interval abs(const Interval &value) {
	if(value.min < 0.0f)
		return value.max < 0.0f ? Interval(-value.max, -value.min)
								: Interval(0.0f, max(-value.min, value.max));
	return value;
}

Interval floor(const Interval &value) { return Interval(floorf(value.min), floorf(value.max)); }

Interval min(const Interval &lhs, const Interval &rhs) {
	return Interval(min(lhs.min, rhs.min), min(lhs.max, rhs.max));
}

Interval max(const Interval &lhs, const Interval &rhs) {
	return Interval(max(lhs.min, rhs.min), max(lhs.max, rhs.max));
}

float intersection(const Interval idir[3], const Interval origin[3], const Box<float3> &box) {
	Interval l1, l2, lmin, lmax;

	l1 = idir[0] * (Interval(box.min.x) - origin[0]);
	l2 = idir[0] * (Interval(box.max.x) - origin[0]);
	lmin = min(l1, l2);
	lmax = max(l1, l2);

	l1 = idir[1] * (Interval(box.min.y) - origin[1]);
	l2 = idir[1] * (Interval(box.max.y) - origin[1]);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	l1 = idir[2] * (Interval(box.min.z) - origin[2]);
	l2 = idir[2] * (Interval(box.max.z) - origin[2]);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	return lmin.min <= lmax.max ? lmin.min : constant::inf;
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

	Box<float3> out(corners[0], corners[0]);
	for(auto corner : corners) {
		out.min = min(out.min, corner);
		out.max = max(out.max, corner);
	}

	return out;
}

#include "game/tile.h"

namespace {
ResourceManager<DTexture> s_gui_textures("data/gui/", ".zar");
ResourceManager<DTexture> s_textures("data/", "");
ResourceManager<game::Tile> s_tiles("data/tiles/", ".tile");
ResourceManager<FontCore> s_font_cores("data/fonts/", ".fnt");
ResourceManager<DTexture> s_font_textures("data/fonts/", "");
}

namespace res {
ResourceManager<DTexture> &guiTextures() { return s_gui_textures; }
ResourceManager<DTexture> &textures() { return s_textures; }
ResourceManager<game::Tile> &tiles() { return s_tiles; }

PFont getFont(const string &name) {
	auto core = s_font_cores[name];
	auto texture = s_font_textures[core->textureName()];
	return make_unique<Font>(std::move(core), std::move(texture));
}
}
