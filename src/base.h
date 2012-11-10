#ifndef BASE_H
#define BASE_H

#include <baselib.h>
#include "rapidxml.hpp"

using namespace baselib;

typedef unsigned int uint;

#include "sys/memory.h"

struct uint2;

extern float g_FloatParam[16];

struct int2
{
	int2(int x, int y) : x(x), y(y) { }
	int2() { }
	explicit operator uint2() const;

	int2 operator+(const int2 rhs) const { return int2(x + rhs.x, y + rhs.y); }
	int2 operator-(const int2 rhs) const { return int2(x - rhs.x, y - rhs.y); }
	int2 operator*(int s) const { return int2(x * s, y * s); }
	int2 operator/(int s) const { return int2(x / s, y / s); }
	int2 operator%(int s) const { return int2(x % s, y % s); }
	int2 operator-() const { return int2(-x, -y); }

	bool operator==(const int2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const int2 &rhs) const { return x != rhs.x || y != rhs.y; }

	int x, y;
};

struct int3
{
	int3(int x, int y, int z) : x(x), y(y), z(z) { }
	int3() { }

	int3 operator+(const int3 rhs) const { return int3(x + rhs.x, y + rhs.y, z + rhs.z); }
	int3 operator-(const int3 rhs) const { return int3(x - rhs.x, y - rhs.y, z - rhs.z); }
	int3 operator*(const int3 rhs) const { return int3(x * rhs.x, y * rhs.y, z * rhs.z); }
	int3 operator*(int s) const { return int3(x * s, y * s, z * s); }
	int3 operator/(int s) const { return int3(x / s, y / s, z / s); }
	int3 operator%(int s) const { return int3(x % s, y % s, z % s); }

	bool operator==(const int3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const int3 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }

	int2 xy() const { return int2(x, y); }
	int2 xz() const { return int2(x, z); }
	int2 yz() const { return int2(y, z); }

	int x, y, z;
};

struct int4
{
	int4(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) { }
	int4() { }

	int4 operator+(const int4 rhs) const { return int4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	int4 operator-(const int4 rhs) const { return int4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	int4 operator*(int s) const { return int4(x * s, y * s, z * s, w * s); }
	int4 operator/(int s) const { return int4(x / s, y / s, z / s, w / s); }

	bool operator==(const int4 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const int4 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

	int x, y, z, w;
};

struct uint2
{
	uint2(uint x, uint y) :x(x), y(y) { }
	uint2() { }
	explicit operator int2() const { return int2((int)x, (int)y); }

	uint2 operator+(const uint2 rhs) const { return uint2(x + rhs.x, y + rhs.y); }
	uint2 operator-(const uint2 rhs) const { return uint2(x - rhs.x, y - rhs.y); }
	
	bool operator==(const uint2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const uint2 &rhs) const { return x != rhs.x || y != rhs.y; }

	uint x, y;
};

inline int2::operator uint2() const { return uint2((uint)x, (uint)y); }

struct float2
{
	float2(float x, float y) : x(x), y(y) { }
	float2(int2 xy) :x(xy.x), y(xy.y) { }
	float2() { }
	explicit operator int2() const { return int2((int)x, (int)y); }

	float2 operator+(const float2 rhs) const { return float2(x + rhs.x, y + rhs.y); }
	float2 operator-(const float2 rhs) const { return float2(x - rhs.x, y - rhs.y); }
	float2 operator*(float s) const { return float2(x * s, y * s); }
	float2 operator/(float s) const { return *this * (1.0f / s); }
	float2 operator-() const { return float2(-x, -y); }
	
	bool operator==(const float2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const float2 &rhs) const { return x != rhs.x || y != rhs.y; }

	float x, y;
};

struct float3
{
	float3(float x, float y, float z) : x(x), y(y), z(z) { }
	float3(int3 xyz) :x(xyz.x), y(xyz.y), z(xyz.z) { }
	float3() { }
	explicit operator int3() const { return int3((int)x, (int)y, (int)z); }

	float3 operator+(const float3 rhs) const { return float3(x + rhs.x, y + rhs.y, z + rhs.z); }
	float3 operator-(const float3 rhs) const { return float3(x - rhs.x, y - rhs.y, z - rhs.z); }
	float3 operator*(float s) const { return float3(x * s, y * s, z * s); }
	float3 operator/(float s) const { return *this * (1.0f / s); }
	float3 operator-() const { return float3(-x, -y, -z); }
	
	bool operator==(const float3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const float3 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }

	float2 xy() const { return float2(x, y); }
	float2 xz() const { return float2(x, z); }
	float2 yz() const { return float2(y, z); }

	float x, y, z;
};

struct float4
{
	float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }
	float4() { }

	float4 operator+(const float4 &rhs) const { return float4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	float4 operator-(const float4 &rhs) const { return float4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	float4 operator*(float s) const { return float4(x * s, y * s, z * s, w * s); }
	float4 operator/(float s) const { return *this * (1.0f / s); }
	float4 operator-() const { return float4(-x, -y, -z, -w); }
	
	bool operator==(const float4 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const float4 &rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

	float x, y, z, w;
};

template <class Type2>
struct Rect
{
	typedef decltype(Type2().x) Type;

	Rect(Type2 min, Type2 max) :min(min), max(max) { }
	Rect(Type minX, Type minY, Type maxX, Type maxY) :min(minX, minY), max(maxX, maxY) { }
	Rect() { }

	Type Width() const { return max.x - min.x; }
	Type Height() const { return max.y - min.y; }
	Type2 Size() const { return max - min; }
	Type2 Center() const { return (max + min) / Type(2); }
	Type SurfaceArea() const { return (max.x - min.x) * (max.y - min.y); }

	Rect operator+(const Type2 &offset) const { return Rect(min + offset, max + offset); }
	Rect operator-(const Type2 &offset) const { return Rect(min - offset, max - offset); }

	Rect operator+(const Rect &rhs) { return Rect(Min(min, rhs.min), Max(max, rhs.max)); }

	bool IsEmpty() const { return max.x <= min.x || max.y <= min.y; }
	bool IsInside(const int2 &point) const {
		return	point.x >= min.x && point.x < max.x &&
				point.y >= min.y && point.y < max.y;
	}

	Type2 min, max;
};

template <class Type3>
struct Box
{
	typedef decltype(Type3().x) Type;

	Box(Type3 min, Type3 max) :min(min), max(max) { }
	Box(Type minX, Type minY, Type minZ, Type maxX, Type maxY, Type maxZ)
		:min(minX, minY, minZ), max(maxX, maxY, maxZ) { }
	Box() { }

	Type Width() const { return max.x - min.x; }
	Type Height() const { return max.y - min.y; }
	Type Depth() const { return max.z - min.z; }
	Type3 Size() const { return max - min; }

	Box operator+(const Type3 &offset) const { return Box(min + offset, max + offset); }
	Box operator-(const Type3 &offset) const { return Box(min - offset, max - offset); }

	Box operator+(const Box &rhs) { return Box(Min(min, rhs.min), Max(max, rhs.max)); }

	bool IsEmpty() const { return max.x <= min.x || max.y <= min.y || max.z <= min.z; }
	bool IsInside(const int3 &point) const {
		return	point.x >= min.x && point.x < max.x &&
				point.y >= min.y && point.y < max.y &&
				point.z >= min.z && point.z < max.z;
	}

	Type3 min, max;
};

template <class T>
bool Overlaps(const Rect<T> &a, const Rect<T> &b) {
	return	(b.min.x < a.max.x && a.min.x < b.max.x) &&
			(b.min.y < a.max.y && a.min.y < b.max.y);
}

template <class T>
bool Overlaps(const Box<T> &a, const Box<T> &b) {
	return	(b.min.x < a.max.x && a.min.x < b.max.x) &&
			(b.min.y < a.max.y && a.min.y < b.max.y) &&
			(b.min.z < a.max.z && a.min.z < b.max.z);
}

typedef Rect<int2> IRect;
typedef Rect<float2> FRect;
typedef Box<int3> IBox;
typedef Box<float3> FBox;

template <class T> inline T Max(T a, T b) { return a > b? a : b; }
template <class T> inline T Min(T a, T b) { return a < b? a : b; }

inline const int2 Min(const int2 &lhs, const int2 &rhs) { return int2(Min(lhs.x, rhs.x), Min(lhs.y, rhs.y)); }
inline const int2 Max(const int2 &lhs, const int2 &rhs) { return int2(Max(lhs.x, rhs.x), Max(lhs.y, rhs.y)); }

inline const int3 Min(const int3 &lhs, const int3 &rhs) { return int3(Min(lhs.x, rhs.x), Min(lhs.y, rhs.y), Min(lhs.z, rhs.z)); }
inline const int3 Max(const int3 &lhs, const int3 &rhs) { return int3(Max(lhs.x, rhs.x), Max(lhs.y, rhs.y), Max(lhs.z, rhs.z)); }

template <class T> inline const T Clamp(T obj, T min, T max) { return Min(Max(obj, min), max); }

template <class T, class T1> inline const T& operator+=(T &a, const T1 &b) { a = a + b; return a; }
template <class T, class T1> inline const T& operator-=(T &a, const T1 &b) { a = a - b; return a; }

template <class T> inline void Swap(T &a, T &b) { T tmp = a; a = b; b = tmp; }

inline int3 AsXZ(int2 pos) { return int3(pos.x, 0, pos.y); }
inline int3 AsXY(int2 pos) { return int3(pos.x, pos.y, 0); }
inline int3 AsXZY(int2 pos, int y) { return int3(pos.x, y, pos.y); }

float Dot(const float2 &a, const float2 &b);
float Dot(const float3 &a, const float3 &b);
float Dot(const float4 &a, const float4 &b);


float LengthSq(const float3&);
float DistanceSq(const float3&, const float3&);

float Length(const float3&);
float Distance(const float3&, const float3&);

float3 WorldToScreen(float3 pos);
int3 WorldToScreen(int3 pos);

float2 ScreenToWorld(float2 pos);
int2 ScreenToWorld(int2 pos);

inline float3 WorldToScreen(float2 pos) { return WorldToScreen(float3(pos.x, 0.0f, pos.y)); }


struct Color
{
	Color(u8 r, u8 g, u8 b, u8 a = 255)
		:r(r), g(g), b(b), a(a) { }
	Color(int r, int g, int b, int a = 255)
		:r(Clamp(r, 0, 255)), g(Clamp(g, 0, 255)), b(Clamp(b, 0, 255)), a(Clamp(a, 0, 255)) { }
	Color(float r, float g, float b, float a = 1.0f)
		:r(Clamp(r * 255.0f, 0.0f, 255.0f)), g(Clamp(g * 255.0f, 0.0f, 255.0f)), b(Clamp(b * 255.0f, 0.0f, 255.0f)), 
		 a(Clamp(a * 255.0f, 0.0f, 255.0f)) { }
	Color(u32 rgba) :rgba(rgba) { }
	Color() { }

	Color operator|(Color rhs) const { return rgba | rhs.rgba; }
	operator float4() const { return float4(r, g, b, a) / 255.0f; }

	union {
		struct { u8 r, g, b, a; };
		u32 rgba;
	};
};

inline Color SwapBR(Color col) {
	return ((col.rgba & 0xff) << 16) | ((col.rgba & 0xff0000) >> 16) | (col.rgba & 0xff00ff00);
}

SERIALIZE_AS_POD(int2)
SERIALIZE_AS_POD(int3)
SERIALIZE_AS_POD(int4)
SERIALIZE_AS_POD(float2)
SERIALIZE_AS_POD(float3)
SERIALIZE_AS_POD(float4)
SERIALIZE_AS_POD(IRect)
SERIALIZE_AS_POD(FRect)
SERIALIZE_AS_POD(IBox)
SERIALIZE_AS_POD(FBox)
SERIALIZE_AS_POD(Color)


#define COUNTOF(array)   ((int)(sizeof(array) / sizeof(array[0])))

void FindFiles(vector<string> &out, const char *dirName, const char *ext, bool recursive = false);
//string OpenFileDialog(bool save);

typedef rapidxml::xml_node<> XMLNode;
typedef rapidxml::xml_attribute<> XMLAttribute;
typedef rapidxml::xml_document<> XMLDocument;

void addAttribute(XMLNode *node, const char *name, float value);
void addAttribute(XMLNode *node, const char *name, int value);
void addAttribute(XMLNode *node, const char *name, const char *value);
int getIntAttribute(XMLNode *node, const char *name);
int getFloatAttribute(XMLNode *node, const char *name);
const char *getStringAttribute(XMLNode *node, const char *name);


#endif
