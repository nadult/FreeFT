#ifndef BASE_H
#define BASE_H

#include <baselib.h>

using namespace baselib;

typedef unsigned int uint;

#include "sys/memory.h"

struct uint2;

struct int2
{
	int2(int x, int y) : x(x), y(y) { }
	int2() { }
	explicit operator uint2() const;

	int2 operator+(const int2 rhs) const { return int2(x + rhs.x, y + rhs.y); }
	int2 operator-(const int2 rhs) const { return int2(x - rhs.x, y - rhs.y); }

	bool operator==(const int2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const int2 &rhs) const { return x != rhs.x || y != rhs.y; }

	int x, y;
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
	float2 operator-() const { return float2(-x, -y); }
	
	bool operator==(const float2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const float2 &rhs) const { return x != rhs.x || y != rhs.y; }

	float x, y;
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

	float x, y, z, w;
};

template <class T> inline T Max(T a, T b) { return a > b? a : b; }
template <class T> inline T Min(T a, T b) { return a < b? a : b; }

template <class T> inline const T Clamp(T obj, T min, T max) { return Min(Max(obj, min), max); }

template <class T> inline void operator+=(T &a, T b) { a = a + b; }
template <class T> inline void operator-=(T &a, T b) { a = a - b; }

template <class T> inline void Swap(T &a, T &b) { T tmp = a; a = b; b = tmp; }

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
SERIALIZE_AS_POD(float2)
SERIALIZE_AS_POD(float4)
SERIALIZE_AS_POD(Color)


#define COUNTOF(array)   (sizeof(array) / sizeof(array[0]))

#endif
