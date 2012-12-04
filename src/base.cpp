#include "base.h"
#include <errno.h>
#include <cstring>
#include <cmath>
#include <cstdio>

const int2 min(const int2 &a, const int2 &b) { return int2(min(a.x, b.x), min(a.y, b.y)); }
const int2 max(const int2 &a, const int2 &b) { return int2(max(a.x, b.x), max(a.y, b.y)); }
const int3 min(const int3 &a, const int3 &b) { return int3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
const int3 max(const int3 &a, const int3 &b) { return int3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }

const float2 min(const float2 &a, const float2 &b) { return float2(min(a.x, b.x), min(a.y, b.y)); }
const float2 max(const float2 &a, const float2 &b) { return float2(max(a.x, b.x), max(a.y, b.y)); }
const float3 min(const float3 &a, const float3 &b) { return float3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
const float3 max(const float3 &a, const float3 &b) { return float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }

float vectorToAngle(const float2 &normalized_vec) {
	float ang = acos(normalized_vec.x);
	return normalized_vec.y < 0? 2.0f * constant::pi -ang : ang;
}

const float2 angleToVector(float radians) {
	float s, c;
	sincosf(radians, &s, &c);
	return float2(c, s);
}

const float2 rotateVector(const float2 &vec, float radians) {
	float s, c;
	sincosf(radians, &s, &c);
	return float2(c * vec.x - s * vec.y, c * vec.y + s * vec.x);
}

float g_FloatParam[16];

/*
typedef float Matrix3[3][3];

bool inverse(const Matrix3 &mat) {
	float3 out[3];

    out[0].x = mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1];
    out[0].y = mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2];
    out[0].z = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    out[1].x = mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2];
    out[1].y = mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0];
    out[1].z = mat[0][2] * mat[1][0] - mat[0][0] * mat[1][2];
    out[2].x = mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0];
    out[2].y = mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1];
    out[2].z = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

	float det = mat[0][0] * out[0].x + mat[0][1] * out[1].x + mat[0][2] * out[2].x;
	//TODO what if det close to 0?
	float idet = 1.0f / det;

	out[0] = out[0] * idet;
	out[1] = out[1] * idet;
	out[2] = out[2] * idet;

	for(int i = 0; i < 3; i++) {
		printf("%f %f %f\n", out[i].x, out[i].y, out[i].z);
	}
	exit(0);
	return 1;
}

float mat[3][3] = {
	{6, 3, 7},
	{0, -7, 6},
	{-6, 3, 7}
};*/

/* World To Screen Matrix:
 * 			    | 6  3  7|
 * 		*	    | 0 -7  6|
 * 			    |-6  3  7|
 *
 * [WX WY WZ]   [SX SY SZ]
 *
 */

float2 worldToScreen(float3 pos) {
	return float2(	6.0f * (pos.x - pos.z),
					3.0f * (pos.x + pos.z) - 7.0f * pos.y);
				//	7.0f * (pos.x + pos.z) + 6.0f * pos.y);
}

int2 worldToScreen(int3 pos) {
	return int2(	6 * (pos.x - pos.z),
					3 * (pos.x + pos.z) - 7 * pos.y);
				//	7 * (pos.x + pos.z) - 6 * pos.y);
}

float2 screenToWorld(float2 pos) {
	float x = pos.x * (1.0f / 12.0f);
	float y = pos.y * (1.0f / 6.0f);

	return float2(y + x, y - x);
}

int2 screenToWorld(int2 pos) {
	int x = pos.x / 12;
	int y = pos.y / 6;

	return int2(y + x, y - x);
}

const IRect worldToScreen(const IBox &bbox) {
	int2 corners[4] = {
			worldToScreen(int3(bbox.max.x, bbox.min.y, bbox.min.z)),
			worldToScreen(int3(bbox.min.x, bbox.min.y, bbox.max.z)),
			worldToScreen(int3(bbox.max.x, bbox.min.y, bbox.max.z)),
			worldToScreen(int3(bbox.min.x, bbox.max.y, bbox.min.z)) };

	return IRect(corners[1].x, corners[3].y, corners[0].x, corners[2].y);
}

float intersection(const Ray &ray, const Box<float3> &box) {
	//TODO: check if works correctly for (+/-)INF
	float3 inv_dir = ray.invDir();
	float3 origin = ray.origin();

	float l1   = inv_dir.x * (box.min.x - origin.x);
	float l2   = inv_dir.x * (box.max.x - origin.x);
	float lmin = min(l1, l2);
	float lmax = max(l1, l2);

	l1   = inv_dir.y * (box.min.y - origin.y);
	l2   = inv_dir.y * (box.max.y - origin.y);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	l1   = inv_dir.z * (box.min.z - origin.z);
	l2   = inv_dir.z * (box.max.z - origin.z);
	lmin = max(min(l1, l2), lmin);
	lmax = min(max(l1, l2), lmax);

	return lmin < lmax? lmin : 1.0f / 0.0f;
}

Ray screenRay(int2 screen_pos) {
	float3 origin = asXZ(screenToWorld((float2)screen_pos));
	float3 dir = float3(-1.0f / 6.0f, -1.0f / 7.0f, -1.0f / 6.0f);
	return Ray(origin, dir / length(dir));
}

float dot(const float2 &a, const float2 &b) { return a.x * b.x + a.y * b.y; }
float dot(const float3 &a, const float3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float dot(const float4 &a, const float4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

float lengthSq(const float2 &v) { return dot(v, v); }
float lengthSq(const float3 &v) { return dot(v, v); }
float distanceSq(const float3 &a, const float3 &b) { return lengthSq(a - b); }
float distanceSq(const float2 &a, const float2 &b) { return lengthSq(a - b); }

float length(const float2 &v) { return sqrt(lengthSq(v)); }
float length(const float3 &v) { return sqrt(lengthSq(v)); }
float distance(const float3 &a, const float3 &b) { return sqrt(distanceSq(a, b)); }
float distance(const float2 &a, const float2 &b) { return sqrt(distanceSq(a, b)); }

bool areAdjacent(const IRect &a, const IRect &b) {
	if(b.min.x < a.max.x && a.min.x < b.max.x)
		return a.max.y == b.min.y || a.min.y == b.max.y;
	if(b.min.y < a.max.y && a.min.y < b.max.y)
		return a.max.x == b.min.x || a.min.x == b.max.x;
	return false;
}

float distanceSq(const Rect<float2> &a, const Rect<float2> &b) {
	float2 p1 = clamp(b.center(), a.min, a.max);
	float2 p2 = clamp(p1, b.min, b.max);
	return distanceSq(p1, p2);
}

MoveVector::MoveVector(const int2 &start, const int2 &end) {
	int2 diff = end - start;
	vec.x = diff.x < 0? -1 : diff.x > 0? 1 : 0;
	vec.y = diff.y < 0? -1 : diff.y > 0? 1 : 0;
	dx = abs(diff.x);
	dy = abs(diff.y);
	ddiag = min(dx, dy);
	dx -= ddiag;
	dy -= ddiag;
}
MoveVector::MoveVector() :vec(0, 0), dx(0), dy(0), ddiag(0) { }

#include <fstream>
#include "rapidxml_print.hpp"

using namespace rapidxml;

void addAttribute(XMLNode *node, const char *name, float value) {
	XMLDocument *doc = node->document();
	char str_value[64];
	snprintf(str_value, sizeof(str_value), "%f", value);
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(str_value));
	node->append_attribute(attrib);
}

void addAttribute(XMLNode *node, const char *name, int value) {
	XMLDocument *doc = node->document();
	char str_value[32];
	sprintf(str_value, "%d", value);
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(str_value));
	node->append_attribute(attrib);
}

void addAttribute(XMLNode *node, const char *name, const char *value) {
	XMLDocument *doc = node->document();
	XMLAttribute *attrib = doc->allocate_attribute(name, doc->allocate_string(value));
	node->append_attribute(attrib);
}

int getIntAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? atoi(attrib->value()) : 0;
}

int getFloatAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? atof(attrib->value()) : 0;
}

const char *getStringAttribute(XMLNode *node, const char *name) {
	XMLAttribute *attrib = node->first_attribute(name);
	return attrib? attrib->value() : 0;
}

void loadXMLDocument(const char *file_name, XMLDocument &doc) {
	DASSERT(file_name);
	Loader ldr(file_name);
	char *xml_string = doc.allocate_string(0, ldr.size());
	ldr.data(xml_string, ldr.size());
	doc.parse<0>(xml_string); 
	
}

void saveXMLDocument(const char *file_name, const XMLDocument &doc) {
	DASSERT(file_name);
	std::fstream file(file_name, std::fstream::out);
	file << doc;
}
