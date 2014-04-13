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

#include "base_math.h"


class XMLNode;
class XMLDocument;

typedef unsigned int uint;

#ifdef _WIN32

const char* strcasestr(const char *a, const char *b);
int strcasecmp(const char *a, const char *b);

#endif

template <class T, class Arg1>
inline constexpr bool isOneOf(const T &value, const Arg1 &arg1) { return value == arg1; }

template <class T, class Arg1, class ...Args>
inline constexpr bool isOneOf(const T &value, const Arg1 &arg1, const Args&... args) { return value == arg1 || isOneOf(value, args...); }

// TODO: finish me
class CString {
public:
	CString(const string &str) :m_str(str.c_str()), m_len((int)str.size()) { }
	CString(const char *str, int len = -1) :m_str(str), m_len(len == -1? strlen(str) : len) { }
	CString() :m_str(nullptr), m_len(0) { }
	explicit operator const char*() const { return m_str; }
	const char *c_str() const { return m_str; }
	bool isValid() const { return m_str != nullptr; }
	int size() const { return m_len; }
	bool isEmpty() const { return m_len == 0; }

	CString operator+(int offset) const {
		DASSERT(offset <= m_len);
		return CString(m_str + offset, m_len - offset);
	}

private:
	const char *m_str;
	int m_len;
};


inline bool operator==(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return a.size() == b.size() && strcmp(a.c_str(), b.c_str()) == 0;
}

inline bool operator!=(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return strcmp(a.c_str(), b.c_str()) != 0;
}

inline bool operator<(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return strcmp(a.c_str(), b.c_str()) < 0;
}

inline bool caseEqual(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return a.size() == b.size() && strcasecmp(a.c_str(), b.c_str()) == 0;
}

inline bool caseNEqual(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return strcasecmp(a.c_str(), b.c_str()) != 0;
}

inline bool caseLess(const CString a, const CString b) {
	DASSERT(a.isValid() && b.isValid());
	return strcasecmp(a.c_str(), b.c_str()) < 0;
}


#include "sys/memory.h"

#define COUNTOF(array)   ((int)(sizeof(array) / sizeof(array[0])))

int fromString(const char *str, const char **strings, int count);

#define DECLARE_ENUM(type, ...) \
	namespace type { enum Type: char { __VA_ARGS__, count }; \
		const char *toString(int); \
		Type fromString(const char*); \
		inline constexpr bool isValid(int val) { return val >= 0 && val < count; } \
	}

#define DEFINE_ENUM(type, ...) \
	namespace type { \
		static const char *s_strings[] = { __VA_ARGS__ }; \
		static_assert(COUNTOF(s_strings) == count, "String count does not match enum count"); \
		const char *toString(int value) { \
			DASSERT(value >= 0 && value < count); \
			return s_strings[value]; \
		} \
		Type fromString(const char *str) { \
			return (Type)::fromString(str, s_strings, count); \
		} }

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
	explicit PodArray(int size) :m_data(nullptr), m_size(0) { resize(size); }
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

class TextFormatter {
public:
	TextFormatter(int size);

	void operator()(const char *format, ...);
	const char *text() const { return m_data.data(); }
	int length() const { return m_offset; }

private:
	PodArray<char> m_data;
	uint m_offset;
};

template <class T>
class ClonablePtr: public unique_ptr<T> {
public:
	ClonablePtr(const ClonablePtr &rhs) :unique_ptr<T>(rhs? rhs->clone() : nullptr) { }
	ClonablePtr(ClonablePtr &&rhs) :unique_ptr<T>(std::move(rhs)) { }
	ClonablePtr(T *ptr) :unique_ptr<T>(ptr) { }
	ClonablePtr() { }

	explicit operator bool() const { return unique_ptr<T>::operator bool(); }
	bool isValid() const { return unique_ptr<T>::operator bool(); }

	void operator=(ClonablePtr &&rhs) { unique_ptr<T>::operator=(std::move(rhs)); }
	void operator=(const ClonablePtr &rhs) {
		if(&rhs == this)
			return;
		T *clone = rhs? rhs->clone() : nullptr;
		unique_ptr<T>::reset(clone);
	}
};

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
	explicit Color(const float4 &c) :Color(float3(c.x, c.y, c.z), c.w) { }
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

class TupleParser;

// These functions expect valid strings and throw on error

bool  toBool(const char *input);
int   toInt(const char *input);

float toFloat(const char *input);
float2 toFloat2(const char *input);
float3 toFloat3(const char *input);
float4 toFloat4(const char *input);

uint  toFlags(const char *input, const char **flags, int num_flags, uint first_flag);

#endif
