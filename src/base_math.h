/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef BASE_MATH_H
#define BASE_MATH_H

#include "base.h"

extern float g_FloatParam[16];

template <class T> inline T max(T a, T b) { return a < b? b : a; }
template <class T> inline T min(T a, T b) { return a > b? b : a; }

float frand();

namespace constant {
	static const float pi		= 3.14159265358979f;
	static const float e		= 2.71828182845905f;
	static const float inf		= 1.0f / 0.0f;
	static const float epsilon	= 0.0001f;
}

struct Interval {
	Interval(float value) :min(value), max(value) { }
	Interval(float min, float max) :min(min), max(max) { }
	Interval() { }

	const Interval operator+(const Interval &rhs) const { return Interval(min + rhs.min, max + rhs.max); }
	const Interval operator-(const Interval &rhs) const { return Interval(min - rhs.max, max - rhs.min); }
	const Interval operator*(const Interval &rhs) const;
	const Interval operator*(float) const;
	const Interval operator/(float val) const { return operator*(1.0f / val); }

	bool isValid() const { return min <= max; }


	float min, max;
};

const Interval abs(const Interval&);
const Interval floor(const Interval&);
const Interval min(const Interval&, const Interval&);
const Interval max(const Interval&, const Interval&);

struct int2
{
	int2(int x, int y) : x(x), y(y) { }
	int2() :x(0), y(0) { }

	int2 operator+(const int2 &rhs) const { return int2(x + rhs.x, y + rhs.y); }
	int2 operator-(const int2 &rhs) const { return int2(x - rhs.x, y - rhs.y); }
	int2 operator*(int s) const { return int2(x * s, y * s); }
	int2 operator/(int s) const { return int2(x / s, y / s); }
	int2 operator%(int s) const { return int2(x % s, y % s); }
	int2 operator-() const { return int2(-x, -y); }

	bool operator==(const int2 &rhs) const { return x == rhs.x && y == rhs.y; }

	int x, y;
};

//TODO: make operations on ints, only store in shorts
struct short2
{
	short2(short x, short y) : x(x), y(y) { }
	short2(const int2 &rhs) :x(rhs.x), y(rhs.y) { }
	short2() :x(0), y(0) { }
	operator const int2() const { return int2(x, y); }

	short2 operator+(const short2 &rhs) const { return short2(x + rhs.x, y + rhs.y); }
	short2 operator-(const short2 &rhs) const { return short2(x - rhs.x, y - rhs.y); }
	short2 operator*(short s) const { return short2(x * s, y * s); }
	short2 operator/(short s) const { return short2(x / s, y / s); }
	short2 operator%(short s) const { return short2(x % s, y % s); }
	short2 operator-() const { return short2(-x, -y); }

	bool operator==(const short2 &rhs) const { return x == rhs.x && y == rhs.y; }

	short x, y;
};

struct int3
{
	int3(int x, int y, int z) : x(x), y(y), z(z) { }
	int3() :x(0), y(0), z(0) { }

	int3 operator+(const int3 &rhs) const { return int3(x + rhs.x, y + rhs.y, z + rhs.z); }
	int3 operator-(const int3 &rhs) const { return int3(x - rhs.x, y - rhs.y, z - rhs.z); }
	int3 operator*(const int3 &rhs) const { return int3(x * rhs.x, y * rhs.y, z * rhs.z); }
	int3 operator*(int s) const { return int3(x * s, y * s, z * s); }
	int3 operator/(int s) const { return int3(x / s, y / s, z / s); }
	int3 operator%(int s) const { return int3(x % s, y % s, z % s); }

	bool operator==(const int3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

	int2 xy() const { return int2(x, y); }
	int2 xz() const { return int2(x, z); }
	int2 yz() const { return int2(y, z); }

	int x, y, z;
};

struct int4
{
	int4(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) { }
	int4() :x(0), y(0), z(0), w(0) { }

	int4 operator+(const int4 rhs) const { return int4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	int4 operator-(const int4 rhs) const { return int4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	int4 operator*(int s) const { return int4(x * s, y * s, z * s, w * s); }
	int4 operator/(int s) const { return int4(x / s, y / s, z / s, w / s); }

	bool operator==(const int4 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

	int x, y, z, w;
};


struct float2
{
	float2(float x, float y) : x(x), y(y) { }
	float2(const int2 &xy) :x(xy.x), y(xy.y) { }
	float2() :x(0.0f), y(0.0f) { }
	explicit operator int2() const { return int2((int)x, (int)y); }

	float2 operator+(const float2 &rhs) const { return float2(x + rhs.x, y + rhs.y); }
	float2 operator-(const float2 &rhs) const { return float2(x - rhs.x, y - rhs.y); }
	float2 operator*(const float2 &rhs) const { return float2(x * rhs.x, y * rhs.y); }
	float2 operator*(float s) const { return float2(x * s, y * s); }
	float2 operator/(float s) const { return *this * (1.0f / s); }
	float2 operator-() const { return float2(-x, -y); }
	
	bool operator==(const float2 &rhs) const { return x == rhs.x && y == rhs.y; }

	float x, y;
};

float vectorToAngle(const float2 &normalized_vector);
const float2 angleToVector(float radians);
const float2 rotateVector(const float2 &vec, float radians);

struct float3
{
	float3(float x, float y, float z) : x(x), y(y), z(z) { }
	float3(const int3 &xyz) :x(xyz.x), y(xyz.y), z(xyz.z) { }
	float3() :x(0.0f), y(0.0f), z(0.0f) { }
	explicit operator int3() const { return int3((int)x, (int)y, (int)z); }

	float3 operator+(const float3 &rhs) const { return float3(x + rhs.x, y + rhs.y, z + rhs.z); }
	float3 operator-(const float3 &rhs) const { return float3(x - rhs.x, y - rhs.y, z - rhs.z); }
	float3 operator*(const float3 &rhs) const { return float3(x * rhs.x, y * rhs.y, z * rhs.z); }
	float3 operator*(float s) const { return float3(x * s, y * s, z * s); }
	float3 operator/(float s) const { return *this * (1.0f / s); }
	float3 operator-() const { return float3(-x, -y, -z); }
	
	void operator*=(float s) { x *= s; y *= s; z *= s; }
	
	bool operator==(const float3 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

	float2 xy() const { return float2(x, y); }
	float2 xz() const { return float2(x, z); }
	float2 yz() const { return float2(y, z); }

	float x, y, z;
};

struct float4
{
	float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }
	float4() :x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }

	float4 operator+(const float4 &rhs) const { return float4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	float4 operator-(const float4 &rhs) const { return float4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	float4 operator*(const float4 &rhs) const { return float4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
	float4 operator*(float s) const { return float4(x * s, y * s, z * s, w * s); }
	float4 operator/(float s) const { return *this * (1.0f / s); }
	float4 operator-() const { return float4(-x, -y, -z, -w); }
	
	bool operator==(const float4 &rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

	float x, y, z, w;
};

struct Matrix3 {
	Matrix3(const float3 &a, const float3 &b, const float3 &c);
	Matrix3() { }

	static const Matrix3 identity();

	const float3 operator*(const float3 &vec) const;
	const float3 &operator[](int idx) const { return v[idx]; }
	float3 &operator[](int idx) { return v[idx]; }

	float3 v[3];
};

const Matrix3 transpose(const Matrix3&);
const Matrix3 inverse(const Matrix3&);

inline const int2 min(const int2 &a, const int2 &b) { return int2(min(a.x, b.x), min(a.y, b.y)); }
inline const int2 max(const int2 &a, const int2 &b) { return int2(max(a.x, b.x), max(a.y, b.y)); }
inline const int3 min(const int3 &a, const int3 &b) { return int3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
inline const int3 max(const int3 &a, const int3 &b) { return int3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }

inline const int2 abs(const int2 &v) { return int2(abs(v.x), abs(v.y)); }
inline const int3 abs(const int3 &v) { return int3(abs(v.x), abs(v.y), abs(v.z)); }

inline const float2 min(const float2 &a, const float2 &b) { return float2(min(a.x, b.x), min(a.y, b.y)); }
inline const float2 max(const float2 &a, const float2 &b) { return float2(max(a.x, b.x), max(a.y, b.y)); }
inline const float3 min(const float3 &a, const float3 &b) { return float3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
inline const float3 max(const float3 &a, const float3 &b) { return float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }

inline const int2 round(const float2 &v) { return int2(v.x + 0.5f, v.y + 0.5f); }
inline const int3 round(const float3 &v) { return int3(v.x + 0.5f, v.y + 0.5f, v.z + 0.5f); }

class Ray {
public:
	Ray(const float3 &origin, const float3 &dir) :m_origin(origin), m_dir(dir) {
		m_inv_dir = float3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
	}
	Ray(const float3 &origin, const float3 &dir, const float3 &idir) :m_origin(origin), m_dir(dir), m_inv_dir(idir) { }
	Ray() { }

	const float3 &dir() const { return m_dir; }
	const float3 &invDir() const { return m_inv_dir; }
	const float3 &origin() const { return m_origin; }
	const float3 at(float t) const { return m_origin + m_dir * t; }
	const Ray operator-() const { return Ray(m_origin, -m_dir, -m_inv_dir); }

private:
	float3 m_origin;
	float3 m_dir;
	float3 m_inv_dir;
};

struct Segment: public Ray {
	Segment(const Ray &ray, float min = -constant::inf, float max = constant::inf);
	Segment(const float3 &source, const float3 &target);
	Segment() :m_min(0.0f), m_max(constant::inf) { }

	float min() const { return m_min; }
	float max() const { return m_max; }

protected:
	float m_min, m_max;
};

class Plane {
public:
	Plane() { }
	Plane(const float3 &normalized_dir, float distance);
	Plane(const float3 &origin, const float3 &normalized_dir);
	Plane(const float3 &a, const float3 &b, const float3 &c);

	const float3 origin() const { return m_dir * m_distance; }
	const float3 &normal() const { return m_dir; }
	const float3 &dir() const { return m_dir; }
	float distance() const { return m_distance; }

protected:
	float3 m_dir;
	float m_distance;
};

const float3 project(const float3 &point, const Plane &plane);

template <class Type2>
struct Rect
{
	typedef decltype(Type2().x) Type;

	template <class TType2>
	explicit Rect(const Rect<TType2> &other) :min(other.min), max(other.max) { }
	explicit Rect(const Type2 &size) :min(0, 0), max(size) { }
	Rect(Type2 min, Type2 max) :min(min), max(max) { }
	Rect(Type minX, Type minY, Type maxX, Type maxY) :min(minX, minY), max(maxX, maxY) { }
	Rect() { }
	static const Rect empty() { return Rect(0, 0, 0, 0); }

	const Rect subRect(const Rect &part) const {
		return Rect(lerp(min.x, max.x, part.min.x),
					lerp(min.y, max.y, part.min.y),
					lerp(min.x, max.x, part.max.x),
					lerp(min.x, max.x, part.max.y));
	}

	Type width() const { return max.x - min.x; }
	Type height() const { return max.y - min.y; }
	Type2 size() const { return max - min; }
	Type2 center() const { return (max + min) / Type(2); }
	Type surfaceArea() const { return (max.x - min.x) * (max.y - min.y); }

	void setWidth(Type width) { max.x = min.x + width; }
	void setHeight(Type height) { max.y = min.y + height; }
	void setSize(const Type2 &size) { max = min + size; }

	Rect operator+(const Type2 &offset) const { return Rect(min + offset, max + offset); }
	Rect operator-(const Type2 &offset) const { return Rect(min - offset, max - offset); }
	Rect operator*(Type scale) const { return Rect(min * scale, max * scale); }

	Rect operator+(const Rect &rhs) { return Rect(::min(min, rhs.min), ::max(max, rhs.max)); }

	bool isEmpty() const { return max.x <= min.x || max.y <= min.y; }
	bool isInside(const Type2 &point) const {
		return	point.x >= min.x && point.x < max.x &&
				point.y >= min.y && point.y < max.y;
	}

	Type2 min, max;
};

template <class Type2>
inline const Rect<Type2> inset(Rect<Type2> rect, const Type2 &tl, const Type2 &br) {
	return Rect<Type2>(rect.min + tl, rect.max - br);
}

template <class Type2>
inline const Rect<Type2> inset(Rect<Type2> rect, const Type2 &inset) {
	return Rect<Type2>(rect.min + inset, rect.max - inset);
}

template <class Type2>
bool operator==(const Rect<Type2> &lhs, const Rect<Type2> &rhs) { return lhs.min == rhs.min && lhs.max == rhs.max; }

template <class Type3>
const Rect<Type3> sum(const Rect<Type3> &a, const Rect<Type3> &b) {
	return Rect<Type3>(min(a.min, b.min), max(a.max, b.max));
}

template <class Type3>
const Rect<Type3> intersection(const Rect<Type3> &a, const Rect<Type3> &b) {
	return Rect<Type3>(max(a.min, b.min), min(a.max, b.max));
}


template <class Type3>
struct Box
{
	typedef decltype(Type3().x) Type;

	template <class TType3>
	explicit Box(const Box<TType3> &other) :min(other.min), max(other.max) { }
	Box(Type3 min, Type3 max) :min(min), max(max) { }
	Box(Type minX, Type minY, Type minZ, Type maxX, Type maxY, Type maxZ)
		:min(minX, minY, minZ), max(maxX, maxY, maxZ) { }
	Box() { }
	static const Box empty() { return Box(0, 0, 0, 0, 0, 0); }

	Type width() const { return max.x - min.x; }
	Type height() const { return max.y - min.y; }
	Type depth() const { return max.z - min.z; }
	Type3 size() const { return max - min; }
	Type3 center() const { return (max + min) / Type(2); }

	Box operator+(const Type3 &offset) const { return Box(min + offset, max + offset); }
	Box operator-(const Type3 &offset) const { return Box(min - offset, max - offset); }

	bool isEmpty() const { return max.x <= min.x || max.y <= min.y || max.z <= min.z; }
	bool isInside(const Type3 &point) const {
		return	point.x >= min.x && point.x < max.x &&
				point.y >= min.y && point.y < max.y &&
				point.z >= min.z && point.z < max.z;
	}

	void getCorners(Type3 corners[8]) const {
		for(int n = 0; n < 8; n++) {
			corners[n].x = (n & 4? min : max).x;
			corners[n].y = (n & 2? min : max).y;
			corners[n].z = (n & 1? min : max).z;
		}
	}

	const Type3 closestPoint(const Type3 &point) const {
		return Type3(
			point.x < min.x? min.x : point.x > max.x? max.x : point.x,
			point.y < min.y? min.y : point.y > max.y? max.y : point.y,
			point.z < min.z? min.z : point.z > max.z? max.z : point.z);
	}

	bool operator==(const Box &rhs) const { return min == rhs.min && max == rhs.max; }

	Type3 min, max;
};

template <class Type3>
bool operator==(const Box<Type3> &lhs, const Box<Type3> &rhs) { return lhs.min == rhs.min && lhs.max == rhs.max; }


template <class Type3>
const Box<Type3> sum(const Box<Type3> &a, const Box<Type3> &b) {
	return Box<Type3>(min(a.min, b.min), max(a.max, b.max));
}

template <class Type3>
const Box<Type3> intersection(const Box<Type3> &a, const Box<Type3> &b) {
	return Box<Type3>(max(a.min, b.min), min(a.max, b.max));
}

const Box<int3> enclosingIBox(const Box<float3>&);
const Box<float3> rotateY(const Box<float3> &box, const float3 &origin, float angle);

// returns infinity if doesn't intersect
float intersection(const Ray &ray, const Box<float3> &box);
float intersection(const Interval idir[3], const Interval origin[3], const Box<float3> &box);
float intersection(const Segment &segment, const Box<float3> &box);

template <class T>
bool areOverlapping(const Rect<T> &a, const Rect<T> &b) {
	return	(b.min.x < a.max.x && a.min.x < b.max.x) &&
			(b.min.y < a.max.y && a.min.y < b.max.y);
}

inline bool areOverlapping(const Rect<int2> &a, const Rect<int2> &b) {
	return ( ((b.min.x - a.max.x) & (a.min.x - b.max.x)) & ((b.min.y - a.max.y) & (a.min.y - b.max.y)) ) >> 31;
}

template <class T>
bool areOverlapping(const Rect<T> &a, const Rect<T> &b);

template <class T>
bool areOverlapping(const Box<T> &a, const Box<T> &b);

// TODO: support for overlapping boxes
template <class Type3>
int drawingOrder(const Box<Type3> &a, const Box<Type3> &b) {
	//DASSERT(!areOverlapping(box, box));

	int y_ret = a.max.y <= b.min.y? -1 : b.max.y <= a.min.y? 1 : 0;
	if(y_ret)
		return y_ret;

	int x_ret = a.max.x <= b.min.x? -1 : b.max.x <= a.min.x? 1 : 0;
	if(x_ret)
		return x_ret;

	int z_ret = a.max.z <= b.min.z? -1 : b.max.z <= a.min.z? 1 : 0;
	return z_ret;
}

bool areAdjacent(const Rect<int2>&, const Rect<int2>&);
float distanceSq(const Rect<float2>&, const Rect<float2>&);
float distanceSq(const Box<float3>&, const Box<float3>&);
inline float distance(const Box<float3> &a, const Box<float3> &b) { return sqrtf(distanceSq(a, b)); }
	
bool isInsideFrustum(const float3 &eye_pos, const float3 &eye_dir, float min_dot, const Box<float3> &box);

typedef Rect<int2> IRect;
typedef Rect<float2> FRect;
typedef Box<int3> IBox;
typedef Box<float3> FBox;

template <class T> inline const T clamp(T obj, T min, T max) { return ::min(::max(obj, min), max); }

template <class T, class T1> inline const T& operator+=(T &a, const T1 &b) { a = a + b; return a; }
template <class T, class T1> inline const T& operator-=(T &a, const T1 &b) { a = a - b; return a; }

inline int3 asXZ(const int2 &pos) { return int3(pos.x, 0, pos.y); }
inline int3 asXY(const int2 &pos) { return int3(pos.x, pos.y, 0); }
inline int3 asXZY(const int2 &pos, int y) { return int3(pos.x, y, pos.y); }
inline int3 asXYZ(const int2 &pos, int z) { return int3(pos.x, pos.y, z); }

inline float3 asXZ(const float2 &pos) { return float3(pos.x, 0, pos.y); }
inline float3 asXY(const float2 &pos) { return float3(pos.x, pos.y, 0); }
inline float3 asXZY(const float2 &pos, float y) { return float3(pos.x, y, pos.y); }
inline float3 asXYZ(const float2 &pos, float z) { return float3(pos.x, pos.y, z); }

float dot(const float2 &a, const float2 &b);
float dot(const float3 &a, const float3 &b);
float dot(const float4 &a, const float4 &b);
const float3 cross(const float3 &a, const float3 &b);

float lengthSq(const float2&);
float lengthSq(const float3&);
float distanceSq(const float3&, const float3&);
float distanceSq(const float2&, const float2&);

float length(const float2&);
float length(const float3&);
float distance(const float3&, const float3&);
float distance(const float2&, const float2&);

template <class TVector, class TScalar>
inline const TVector lerp(const TVector &a, const TVector &b, const TScalar &delta) {
	DASSERT(delta >= TScalar(0) && delta <= TScalar(1));
	return (b - a) * delta + a;
}

template <class Vec>
const Vec normalized(const Vec &vec) { return vec / length(vec); }

float angleDistance(float ang1, float ang2);
float blendAngles(float initial, float target, float step);

const float2 worldToScreen(const float3 &pos);
const int2 worldToScreen(const int3 &pos);

const float2 screenToWorld(const float2 &pos);
const int2 screenToWorld(const int2 &pos);

const Ray screenRay(const int2 &screen_pos);

template <class Type3>
const Rect<decltype(Type3().xy())> worldToScreen(const Box<Type3> &bbox) {
	typedef decltype(Type3().xy()) Type2;
	Type2 corners[4] = {
			worldToScreen(Type3(bbox.max.x, bbox.min.y, bbox.min.z)),
			worldToScreen(Type3(bbox.min.x, bbox.min.y, bbox.max.z)),
			worldToScreen(Type3(bbox.max.x, bbox.min.y, bbox.max.z)),
			worldToScreen(Type3(bbox.min.x, bbox.max.y, bbox.min.z)) };

	return Rect<Type2>(corners[1].x, corners[3].y, corners[0].x, corners[2].y);
}

inline float2 worldToScreen(const float2 &pos) { return worldToScreen(float3(pos.x, 0.0f, pos.y)); }

// Plane touches the sphere which encloses the box
vector<float3> genPointsOnPlane(const FBox &box, const float3 &dir, int density, bool outside);
vector<float3> genPoints(const FBox &bbox, int density);

void findPerpendicular(const float3 &v1, float3 &v2, float3 &v3);
const float3 perturbVector(const float3 &vec, float rand1, float rand2, float strength);

#endif

