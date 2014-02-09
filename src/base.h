/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef BASE_H
#define BASE_H

#include <baselib.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace baselib;
using std::swap;
using std::pair;
using std::make_pair;
using std::unique_ptr;

class XMLNode;
class XMLDocument;

typedef unsigned int uint;

#include "sys/memory.h"

#define COUNTOF(array)   ((int)(sizeof(array) / sizeof(array[0])))

extern float g_FloatParam[16];

template <class T> inline T max(T a, T b) { return a < b? b : a; }
template <class T> inline T min(T a, T b) { return a > b? b : a; }

namespace constant {
	static const float pi		= 3.14159265358979f;
	static const float e		= 2.71828182845905f;
	static const float inf		= 1.0f / 0.0f;
	static const float epsilon	= 0.0001f;
}

//TODO: XOR Lists?
struct ListNode {
	ListNode() :next(-1), prev(-1) { }

	int next, prev;
};

struct List {
	List() :head(-1), tail(-1) { }
	bool isEmpty() const { return head == -1; }

	int head, tail;
};

//TODO: add functions to remove head / tail

template <class Object, ListNode Object::*member, class Container>
void listInsert(Container &container, List &list, int idx) __attribute__((noinline));

template <class Object, ListNode Object::*member, class Container>
void listRemove(Container &container, List &list, int idx) __attribute__((noinline));

template <class Object, ListNode Object::*member, class Container>
int freeListAlloc(Container &container, List &free_list) __attribute__((noinline));

// Assumes that node is disconnected
template <class Object, ListNode Object::*member, class Container>
void listInsert(Container &container, List &list, int idx) {
	DASSERT(idx >= 0 && idx < (int)container.size());
	ListNode &node = container[idx].*member;
	DASSERT(node.prev == -1 && node.next == -1);

	node.next = list.head;
	if(list.head == -1)
		list.tail = idx;
	else
		(container[list.head].*member).prev = idx;
	list.head = idx;
}

// Assumes that node is on this list
template <class Object, ListNode Object::*member, class Container>
void listRemove(Container &container, List &list, int idx) {
	DASSERT(idx >= 0 && idx < (int)container.size());
	ListNode &node = container[idx].*member;
	int prev = node.prev, next = node.next;

	if(prev == -1) {
		list.head = next;
	}
	else {
		(container[node.prev].*member).next = next;
		node.prev = -1;
	}

	if(next == -1) {
		list.tail = prev;
	}
	else {
		(container[next].*member).prev = prev;
		node.next = -1;
	}
}

// Assumes that node is disconnected
template <class Object, ListNode Object::*member, class Container>
int freeListAlloc(Container &container, List &free_list) {
	int idx;

	if(free_list.isEmpty()) {
		container.emplace_back();
		idx = (int)container.size() - 1;
	}
	else {
		idx = free_list.head;
		listRemove<Object, member>(container, free_list, idx);
	}

	return idx;
}

template <class Object>
class LinkedVector
{
public:
	typedef pair<ListNode, Object> Elem;
	LinkedVector() :m_list_size(0) { }

	Object &operator[](int idx) { return m_objects[idx].second; }
	const Object &operator[](int idx) const { return m_objects[idx].second; }
	int size() const { return m_objects.size(); }
	int listSize() const { return m_list_size; }

	int alloc() {
		int idx = freeListAlloc<Elem, &Elem::first>(m_objects, m_free);
		listInsert<Elem, &Elem::first>(m_objects, m_active, idx);
		m_list_size++;
		return idx;
	}
	
	void free(int idx) {
		DASSERT(idx >= 0 && idx < m_objects.size());
		listRemove<Elem, &Elem::first>(m_objects, m_active, idx);
		listInsert<Elem, &Elem::first>(m_objects, m_free, idx);
		m_list_size--;
	}

	int next(int idx) const { return m_objects[idx].first.next; }
	int prev(int idx) const { return m_objects[idx].first.prev; }

	int head() const { return m_active.head; }
	int tail() const { return m_active.tail; }

protected:
	vector<Elem> m_objects;
	List m_active, m_free;
	int m_list_size;
};


// Very simple and efficent vector for POD Types; Use with care:
// - user is responsible for initializing the data
// - when resizing, data is destroyed
// TODO: derive from PodArrayBase, resize, and serialize can be shared (modulo sizeof(T) multiplier)
template <class T>
class PodArray {
public:
	PodArray() :m_data(nullptr), m_size(0) { }
	PodArray(int size) :m_data(nullptr), m_size(0) { resize(size); }
	PodArray(const PodArray &rhs) :m_size(rhs.m_size) {
		m_data = (T*)sys::alloc(m_size * sizeof(T));
		memcpy(m_data, rhs.m_data, sizeof(T) * m_size);
	}
	PodArray(const T *data, int data_size) :m_size(data_size) {
		m_data = (T*)sys::alloc(m_size * sizeof(T));
		memcpy(m_data, data, data_size * sizeof(T));
	}
	PodArray(PodArray &&rhs) :m_size(rhs.m_size), m_data(rhs.m_data) {
		rhs.m_data = nullptr;
		rhs.m_size = 0;
	}
	~PodArray() {
		resize(0);
	}

	void operator=(const PodArray &rhs) {
		if(&rhs == this)
			return;
		resize(rhs.m_size);
		memcpy(m_data, rhs.m_data, rhs.m_size);
	}
	void operator=(PodArray &&rhs) {
		if(&rhs == this)
			return;
		clear();
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_data = nullptr;
		rhs.m_size = 0;
	}
	void load(Stream &sr) __attribute__((noinline));
	void save(Stream &sr) const __attribute__((noinline));
	void resize(int new_size) __attribute__((noinline));

	void swap(PodArray &rhs) {
		::swap(m_data, rhs.m_data);
		::swap(m_size, rhs.m_size);
	}

	void clear() {
		m_size = 0;
		sys::free(m_data);
		m_data = nullptr;
	}
	bool isEmpty() const {
		return m_size == 0;
	}

	T *data() { return m_data; }
	const T *data() const { return m_data; }
	
	T *end() { return m_data + m_size; }
	const T *end() const { return m_data + m_size; }

	T &operator[](int idx) { return m_data[idx]; }
	const T&operator[](int idx) const { return m_data[idx]; }

	int size() const { return m_size; }
	int dataSize() const { return m_size * (int)sizeof(T); }

private:
	T *m_data;
	int m_size;
};

template <class T>
void PodArray<T>::load(Stream &sr) {
	i32 size;
	sr >> size;
	ASSERT(size >= 0);

	resize(size);
	if(m_data)
		sr.loadData(m_data, sizeof(T) * m_size);
}

template <class T>
void PodArray<T>::save(Stream &sr) const {
	sr << m_size;
	if(m_data)
		sr.saveData(m_data, sizeof(T) * m_size);
}

template <class T>
void PodArray<T>::resize(int new_size) {
	DASSERT(new_size >= 0);
	if(m_size == new_size)
		return;

	clear();
	m_size = new_size;
	if(new_size)
		m_data = (T*)sys::alloc(new_size * sizeof(T));
}

class BitVector
{
public:
	typedef u32 base_type;
	enum {
		base_shift = 5,
		base_size = 32,
	};

	struct Bit {
		Bit(base_type &base, int bit_index) :base(base), bit_index(bit_index) { }
		operator bool() const { return base & (base_type(1) << bit_index); }

		void operator=(bool value) {
			base = (base & ~(base_type(1) << bit_index)) | ((base_type)value << bit_index);
		}

	protected:
		base_type &base;
		int bit_index;
	};

	BitVector(int size = 0);
	void resize(int new_size, bool clear_value = false);

	int size() const { return m_size; }
	int baseSize() const { return m_data.size(); }

	void clear(bool value);

	const PodArray<base_type> &data() const { return m_data; }
	PodArray<base_type> &data() { return m_data; }

	bool operator[](int idx) const {
		return m_data[idx >> base_shift] & (1 << (idx & (base_size - 1)));
	}

	Bit operator[](int idx) {
		return Bit(m_data[idx >> base_shift], idx & (base_size - 1));
	}

	bool any(int base_idx) const {
		return m_data[base_idx] != base_type(0);
	}

	bool all(int base_idx) const {
		return m_data[base_idx] == ~base_type(0);
	}

protected:
	PodArray<base_type> m_data;
	int m_size;
};


struct int2
{
	int2(int x, int y) : x(x), y(y) { }
	int2() { }

	int2 operator+(const int2 &rhs) const { return int2(x + rhs.x, y + rhs.y); }
	int2 operator-(const int2 &rhs) const { return int2(x - rhs.x, y - rhs.y); }
	int2 operator*(int s) const { return int2(x * s, y * s); }
	int2 operator/(int s) const { return int2(x / s, y / s); }
	int2 operator%(int s) const { return int2(x % s, y % s); }
	int2 operator-() const { return int2(-x, -y); }

	bool operator==(const int2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const int2 &rhs) const { return x != rhs.x || y != rhs.y; }

	int x, y;
};

//TODO: make operations on ints, only store in shorts
struct short2
{
	short2(short x, short y) : x(x), y(y) { }
	short2(const int2 &rhs) :x(rhs.x), y(rhs.y) { }
	short2() { }
	operator const int2() const { return int2(x, y); }

	short2 operator+(const short2 &rhs) const { return short2(x + rhs.x, y + rhs.y); }
	short2 operator-(const short2 &rhs) const { return short2(x - rhs.x, y - rhs.y); }
	short2 operator*(short s) const { return short2(x * s, y * s); }
	short2 operator/(short s) const { return short2(x / s, y / s); }
	short2 operator%(short s) const { return short2(x % s, y % s); }
	short2 operator-() const { return short2(-x, -y); }

	bool operator==(const short2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const short2 &rhs) const { return x != rhs.x || y != rhs.y; }

	short x, y;
};

struct int3
{
	int3(int x, int y, int z) : x(x), y(y), z(z) { }
	int3() { }

	int3 operator+(const int3 &rhs) const { return int3(x + rhs.x, y + rhs.y, z + rhs.z); }
	int3 operator-(const int3 &rhs) const { return int3(x - rhs.x, y - rhs.y, z - rhs.z); }
	int3 operator*(const int3 &rhs) const { return int3(x * rhs.x, y * rhs.y, z * rhs.z); }
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


struct float2
{
	float2(float x, float y) : x(x), y(y) { }
	float2(const int2 &xy) :x(xy.x), y(xy.y) { }
	float2() { }
	explicit operator int2() const { return int2((int)x, (int)y); }

	float2 operator+(const float2 &rhs) const { return float2(x + rhs.x, y + rhs.y); }
	float2 operator*(const float2 &rhs) const { return float2(x * rhs.x, y * rhs.y); }
	float2 operator-(const float2 &rhs) const { return float2(x - rhs.x, y - rhs.y); }
	float2 operator*(float s) const { return float2(x * s, y * s); }
	float2 operator/(float s) const { return *this * (1.0f / s); }
	float2 operator-() const { return float2(-x, -y); }
	
	bool operator==(const float2 &rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const float2 &rhs) const { return x != rhs.x || y != rhs.y; }

	float x, y;
};

float vectorToAngle(const float2 &normalized_vector);
const float2 angleToVector(float radians);
const float2 rotateVector(const float2 &vec, float radians);

struct float3
{
	float3(float x, float y, float z) : x(x), y(y), z(z) { }
	float3(const int3 &xyz) :x(xyz.x), y(xyz.y), z(xyz.z) { }
	float3() { }
	explicit operator int3() const { return int3((int)x, (int)y, (int)z); }

	float3 operator+(const float3 &rhs) const { return float3(x + rhs.x, y + rhs.y, z + rhs.z); }
	float3 operator-(const float3 &rhs) const { return float3(x - rhs.x, y - rhs.y, z - rhs.z); }
	float3 operator*(float s) const { return float3(x * s, y * s, z * s); }
	float3 operator/(float s) const { return *this * (1.0f / s); }
	float3 operator-() const { return float3(-x, -y, -z); }
	
	void operator*=(float s) { x *= s; y *= s; z *= s; }
	
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
	Segment(const Ray &ray, float min = -constant::inf, float max = constant::inf)
		:Ray(ray), min(min), max(max) { }
	const Segment operator-() const { return Segment(Ray::operator-(), -max, -min); }

	float min, max;
};


template <class Type2>
struct Rect
{
	typedef decltype(Type2().x) Type;

	template <class TType2>
	explicit Rect(const Rect<TType2> &other) :min(other.min), max(other.max) { }
	Rect(Type2 min, Type2 max) :min(min), max(max) { }
	Rect(Type minX, Type minY, Type maxX, Type maxY) :min(minX, minY), max(maxX, maxY) { }
	Rect() { }
	static const Rect empty() { return Rect(0, 0, 0, 0); }

	Type width() const { return max.x - min.x; }
	Type height() const { return max.y - min.y; }
	Type2 size() const { return max - min; }
	Type2 center() const { return (max + min) / Type(2); }
	Type surfaceArea() const { return (max.x - min.x) * (max.y - min.y); }

	Rect operator+(const Type2 &offset) const { return Rect(min + offset, max + offset); }
	Rect operator-(const Type2 &offset) const { return Rect(min - offset, max - offset); }
	Rect operator*(Type scale) const { return Rect(min * scale, max * scale); }

	Rect operator+(const Rect &rhs) { return Rect(::min(min, rhs.min), ::max(max, rhs.max)); }

	bool isEmpty() const { return max.x <= min.x || max.y <= min.y; }
	bool isInside(const int2 &point) const {
		return	point.x >= min.x && point.x < max.x &&
				point.y >= min.y && point.y < max.y;
	}

	Type2 min, max;
};

template <class Type2>
bool operator==(const Rect<Type2> &lhs, const Rect<Type2> &rhs) { return lhs.min == rhs.min && lhs.max == rhs.max; }
template <class Type2>
bool operator!=(const Rect<Type2> &lhs, const Rect<Type2> &rhs) { return lhs.min != rhs.min || lhs.max != rhs.max; }

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

	bool operator==(const Box &rhs) const { return min == rhs.min && max == rhs.max; }

	Type3 min, max;
};

template <class Type3>
bool operator==(const Box<Type3> &lhs, const Box<Type3> &rhs) { return lhs.min == rhs.min && lhs.max == rhs.max; }
template <class Type3>
bool operator!=(const Box<Type3> &lhs, const Box<Type3> &rhs) { return lhs.min != rhs.min || lhs.max != rhs.max; }


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

// returns infinity if doesn't intersect
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

inline float3 asXZ(const float2 &pos) { return float3(pos.x, 0, pos.y); }
inline float3 asXY(const float2 &pos) { return float3(pos.x, pos.y, 0); }
inline float3 asXZY(const float2 &pos, float y) { return float3(pos.x, y, pos.y); }

float dot(const float2 &a, const float2 &b);
float dot(const float3 &a, const float3 &b);
float dot(const float4 &a, const float4 &b);


float lengthSq(const float2&);
float lengthSq(const float3&);
float distanceSq(const float3&, const float3&);
float distanceSq(const float2&, const float2&);

float length(const float2&);
float length(const float3&);
float distance(const float3&, const float3&);
float distance(const float2&, const float2&);

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

struct MoveVector {
	MoveVector(const int2 &start, const int2 &end);
	MoveVector();

	int2 vec;
	int dx, dy, ddiag;
};



struct Color
{
	explicit Color(u8 r, u8 g, u8 b, u8 a = 255)
		:r(r), g(g), b(b), a(a) { }
	explicit Color(int r, int g, int b, int a = 255)
		:r(clamp(r, 0, 255)), g(clamp(g, 0, 255)), b(clamp(b, 0, 255)), a(clamp(a, 0, 255)) { }
	explicit Color(float r, float g, float b, float a = 1.0f)
		:r(clamp(r * 255.0f, 0.0f, 255.0f)), g(clamp(g * 255.0f, 0.0f, 255.0f)), b(clamp(b * 255.0f, 0.0f, 255.0f)), 
		 a(clamp(a * 255.0f, 0.0f, 255.0f)) { }
	explicit Color(const float3 &c, float a = 1.0f)
		:r(clamp(c.x * 255.0f, 0.0f, 255.0f)), g(clamp(c.y * 255.0f, 0.0f, 255.0f)), b(clamp(c.z * 255.0f, 0.0f, 255.0f)), 
		 a(clamp(a * 255.0f, 0.0f, 255.0f)) { }
	Color(u32 rgba) :rgba(rgba) { }
	Color(Color col, u8 alpha) :rgba((col.rgba & rgb_mask) | ((u32)alpha << 24)) { }
	Color() = default;

	Color operator|(Color rhs) const { return rgba | rhs.rgba; }
	operator float4() const { return float4(r, g, b, a) * (1.0f / 255.0f); }
	operator float3() const { return float3(r, g, b) * (1.0f / 255.0f); }

   	//TODO: endianess...

	enum {
		white 		= 0xffffffffu,
		gray		= 0xff7f7f7fu,
		yellow		= 0xff00ffffu,
		red			= 0xff0000ffu,
		black 		= 0xff000000u,
		gui_dark	= 0xffe86f25u,
		gui_medium	= 0xffe77738u,
		gui_light	= 0xffe7864cu,
		gui_popup	= 0xffffa060u,
		transparent = 0x00000000u,
	};

	enum {
		alpha_mask	= 0xff000000u,
		rgb_mask	= 0x00ffffffu,
	};

	union {
		struct { u8 r, g, b, a; };
		u32 rgba;
	};
};

inline bool operator==(const Color &lhs, const Color &rhs) { return lhs.rgba == rhs.rgba; }
inline bool operator!=(const Color &lhs, const Color &rhs) { return lhs.rgba != rhs.rgba; }

inline Color swapBR(Color col) {
	return ((col.rgba & 0xff) << 16) | ((col.rgba & 0xff0000) >> 16) | (col.rgba & 0xff00ff00);
}

void compress(const PodArray<char> &in, PodArray<char> &out, bool hc);
void decompress(const PodArray<char> &in, PodArray<char> &out);

SERIALIZE_AS_POD(short2)
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


namespace gfx {
	class SceneRenderer;
	class Texture;
	class Font;
	class DTexture;

	typedef Ptr<Font> PFont;
	typedef Ptr<DTexture> PTexture;
}

namespace game {
	class Tile;
	class Sprite;
	class Entity;
	class TileMap;
	class EntityMap;
	typedef Ptr<Sprite> PSprite;
	typedef std::unique_ptr<Entity> PEntity;
}

#endif
