// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of libfwk. See license.txt for details.

#pragma once

#include <fwk/math_base.h>
#include <fwk/pod_vector.h>
#include <fwk/str.h>
#include <fwk/sys_base.h>

using namespace fwk;

template <int size> struct PodData { char data[size]; };
template <int size> inline constexpr bool fwk::is_flat_data<PodData<size>> = true;

template <class T> const PodData<sizeof(T)> &asPod(const T &value) {
	return reinterpret_cast<PodData<sizeof(T)> &>(value);
}
template <class T> PodData<sizeof(T)> &asPod(T &value) {
	return reinterpret_cast<PodData<sizeof(T)> &>(value);
}

DEFINE_ENUM(StreamMode, loading, saving);

class MemoryStream {
  public:
	static constexpr int default_max_string_size = 64 * 1024 * 1024;
	static constexpr int default_max_vector_size = 1024 * 1024 * 1024;

	// Do not call directly, use memoryLoader & memorySaver functions
	MemoryStream(CSpan<char>);
	MemoryStream(Span<char>);
	MemoryStream(PodVector<char>, bool is_loading);

	MemoryStream(MemoryStream &&);
	void operator=(MemoryStream &&);
	~MemoryStream();

	void clear();
	void free();
	// Returns buffer and clears
	PodVector<char> extractBuffer();
	bool bufferUsed() const { return m_data == m_buffer.data(); }

	// Makes sense only for saving streams
	void reserve(int new_capacity);

	CSpan<char> data() const { return {m_data, m_size}; }
	int size() const { return m_size; }
	int capacity() const { return m_capacity; }
	int capacityLeft() const { return m_capacity - m_size; }
	int pos() const { return m_pos; }
	bool atEnd() const { return m_pos == m_size; }

	// Saving streams are always valid
	bool valid() const { return m_is_valid; }

	bool isLoading() const { return m_is_loading; }
	bool isSaving() const { return !m_is_loading; }

	void saveData(CSpan<char>);
	void loadData(Span<char>);

	// It's illegal to seek past the end
	void seek(int pos);

	template <class TSpan, class T = SpanBase<TSpan>> void saveData(const TSpan &data) {
		saveData(cspan(data).template reinterpret<char>());
	}
	template <class TSpan, class T = SpanBase<TSpan>, EnableIf<!is_const<T>>...>
	void loadData(TSpan &&data) {
		loadData(span(data).template reinterpret<char>());
	}

	// -------------------------------------------------------------------------------------------
	// ---  Saving/loading objects   -------------------------------------------------------------

	template <class T, EnableIf<is_flat_data<T>>...> MemoryStream &operator<<(const T &obj) {
		return saveData(cspan(&obj, 1)), *this;
	}
	template <class T, EnableIf<!is_flat_data<T>>...> MemoryStream &operator<<(const T &obj) {
		obj.save(*this);
		return *this;
	}

	template <class T, EnableIf<is_flat_data<T>>...> MemoryStream &operator>>(T &obj) {
		return loadData(span(&obj, 1)), *this;
	}
	template <class T, EnableIf<!is_flat_data<T>>...> MemoryStream &operator>>(T &obj) {
		obj.load(*this);
		return *this;
	}
	template <class T> MemoryStream &operator>>(Maybe<T> &obj) {
		char exists;
		*this >> exists;
		if(exists) {
			T tmp;
			*this >> tmp;
			obj = tmp;
		} else {
			obj = {};
		}
		return *this;
	}

	template <class T> MemoryStream &operator<<(const Maybe<T> &obj) {
		*this << char(obj ? 1 : 0);
		if(obj)
			*this << *obj;
		return *this;
	}

	// TODO: better name
	// TODO: write/read directly from span; double memcpy is unnecessary
	template <class... Args, EnableIf<(... && is_flat_data<Args>)>...>
	void pack(const Args &... args) {
		char buffer[(sizeof(Args) + ...)];
		int offset = 0;
		((memcpy(buffer + offset, &args, sizeof(Args)), offset += sizeof(Args)), ...);
		saveData(buffer);
	}

	template <class... Args, EnableIf<(... && is_flat_data<Args>)>...> void unpack(Args &... args) {
		char buffer[(sizeof(Args) + ...)];
		loadData(buffer);
		int offset = 0;
		((memcpy(&args, buffer + offset, sizeof(Args)), offset += sizeof(Args)), ...);
	}

	// -------------------------------------------------------------------------------------------
	// ---  Saving/loading strings & vectors of POD objects  -------------------------------------

	// If size < 254: saves single byte, else saves 5 or 9 bytes
	void saveSize(i64);
	void saveString(CSpan<char>);
	void saveVector(CSpan<char>, int element_size = 1);

	i64 loadSize();
	string loadString(int max_size = default_max_string_size);
	// Terminating zero will be added as well
	int loadString(Span<char>);

	MemoryStream &operator<<(const string &);
	MemoryStream &operator>>(string &);

	template <class T, EnableIf<is_flat_data<T>>...> void saveVector(CSpan<T> vec) {
		saveVector(vec.template reinterpret<char>(), sizeof(T));
	}

	PodVector<char> loadVector(int max_size = default_max_vector_size, int element_size = 1);

	template <class T, EnableIf<is_flat_data<T>>...>
	PodVector<T> loadVector(int max_size = default_max_vector_size) {
		auto out = loadVector(max_size, sizeof(T));
		return move(reinterpret_cast<PodVector<T> &>(out)); // TODO: use PodVector::reinterpet
	}

	// Serializes signatures; While saving, it simply writes it to a stream
	// When loading, it will report an error if signature is not exactly matched
	void signature(u32);
	// Max length: 32
	void signature(Str);

  private:
	void raise(ZStr);

	// TODO: drop PodVector, store just single pointer
	PodVector<char> m_buffer;
	char *m_data;
	int m_pos = 0, m_size = 0, m_capacity = 0;
	bool m_is_valid = true;
	bool m_is_loading;
};

// Will keep reference to passed buffer
MemoryStream memoryLoader(CSpan<char>);
MemoryStream memoryLoader(vector<char>);
MemoryStream memoryLoader(PodVector<char>);

// Will keep reference to passed buffer. Will allocate memory when saved data
// won't fit in passed buffer.
MemoryStream memorySaver(Span<char>);
// Data in buffer will be lost
MemoryStream memorySaver(int capacity = 256);
MemoryStream memorySaver(PodVector<char> buffer);
