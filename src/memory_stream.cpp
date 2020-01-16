// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of libfwk. See license.txt for details.

#include "memory_stream.h"

#include <cstring>
#include <errno.h>
#include <fwk/math_base.h>
#include <fwk/sys/assert.h>
#include <fwk/sys/expected.h>
#include <limits>

using namespace fwk;

MemoryStream::MemoryStream(CSpan<char> data)
	: m_data((char *)data.data()), m_capacity(data.size()), m_size(data.size()),
	  m_is_loading(true) {}
MemoryStream::MemoryStream(Span<char> data)
	: m_data(data.data()), m_capacity(data.size()), m_size(0), m_is_loading(false) {}
MemoryStream::MemoryStream(PodVector<char> buffer, bool is_loading)
	: m_buffer(move(buffer)), m_data(buffer.data()), m_capacity(buffer.size()),
	  m_is_loading(is_loading) {
	m_size = is_loading ? m_buffer.size() : 0;
}

MemoryStream::MemoryStream(MemoryStream &&rhs)
	: m_buffer(move(rhs.m_buffer)), m_data(rhs.m_data), m_pos(rhs.m_pos), m_size(rhs.m_size),
	  m_capacity(rhs.m_capacity), m_is_valid(rhs.m_is_valid), m_is_loading(rhs.m_is_loading) {
	rhs.free();
}

// TODO: auto reconstruct
void MemoryStream::operator=(MemoryStream &&rhs) {
	m_buffer = move(rhs.m_buffer);
	m_data = rhs.m_data;
	m_pos = rhs.m_pos;
	m_size = rhs.m_size;
	m_capacity = rhs.m_capacity;
	m_is_valid = rhs.m_is_valid;
	rhs.free();
}

MemoryStream::~MemoryStream() = default;

void MemoryStream::clear() {
	m_size = m_pos = 0;
	m_is_valid = true;
}

void MemoryStream::free() {
	m_buffer.free();
	m_data = nullptr;
	m_pos = m_size = m_capacity = 0;
	m_is_valid = true;
}

PodVector<char> MemoryStream::extractBuffer() {
	auto out = move(m_buffer);
	free();
	return out;
}

void MemoryStream::reserve(int new_capacity) {
	DASSERT(!m_is_loading);

	if(new_capacity <= m_capacity)
		return;
	PodVector<char> new_buffer(BaseVector::insertCapacity(m_capacity, 1, new_capacity));
	copy(new_buffer, data());
	m_data = new_buffer.data();
	m_capacity = new_buffer.size();
	m_buffer.swap(new_buffer);
}

void MemoryStream::saveData(CSpan<char> data) {
	PASSERT(!m_is_loading);
	if(m_pos + data.size() > m_capacity)
		reserve(m_pos + data.size());
	memcpy(m_data + m_pos, data.data(), data.size());
	m_pos += data.size();
}

void MemoryStream::loadData(Span<char> data) {
	PASSERT(m_is_loading);

	if(!m_is_valid) {
		fill(data, 0);
		return;
	}
	if(m_pos + data.size() > m_size) {
		raise(format("Reading past the end: % + % > %", m_pos, data.size(), m_size));
		fill(data, 0);
		return;
	}

	copy(data, cspan(m_data + m_pos, data.size()));
	m_pos += data.size();
}

void MemoryStream::seek(int pos) {
	DASSERT(pos >= 0 && pos <= m_size);
	m_pos = pos;
}

void MemoryStream::saveSize(i64 size) {
	DASSERT(size >= 0);
	if(size < 254)
		*this << u8(size);
	else if(size <= UINT_MAX)
		pack(u8(254), u32(size));
	else
		pack(u8(255), size);
}

void MemoryStream::saveString(CSpan<char> str) {
	saveSize(str.size());
	saveData(str);
}

void MemoryStream::saveVector(CSpan<char> vec, int element_size) {
	DASSERT(vec.size() % element_size == 0);
	saveSize(vec.size() / element_size);
	saveData(vec);
}

static int decodeString(Str str, Span<char> buf) {
	int len = 0;
	int bufSize = buf.size() - 1;

	for(int n = 0; n < str.size() && len < bufSize; n++) {
		if(str[n] == '\\') {
			if(bufSize - len < 2)
				goto END;

			buf[len++] = '\\';
			buf[len++] = '\\';
		} else if(str[n] >= 32 && str[n] < 127) {
			buf[len++] = str[n];
		} else {
			if(bufSize - len < 4)
				goto END;

			buf[len++] = '\\';
			unsigned int code = (u8)str[n];
			buf[len++] = '0' + code / 64;
			buf[len++] = '0' + (code / 8) % 8;
			buf[len++] = '0' + code % 8;
		}
	}

END:
	buf[len++] = 0;
	return len;
}

void MemoryStream::raise(ZStr message) {
	// TODO: proper error handling
	FWK_FATAL("Error while loading data from MemoryStream at position %d/%d: %s", m_pos, size(),
			  message.c_str());
	m_is_valid = false;
}

i64 MemoryStream::loadSize() {
	if(!m_is_valid)
		return 0;

	i64 out = 0;
	{
		u8 small;
		*this >> small;
		if(small == 254) {
			u32 len32;
			*this >> len32;
			out = len32;
		} else if(small == 255) {
			*this >> out;
		} else {
			out = small;
		}
	}

	if(out < 0) {
		raise(format("Invalid length: %", out));
		m_is_valid = false;
		return 0;
	}

	return out;
}

string MemoryStream::loadString(int max_size) {
	auto size = loadSize();
	if(size > max_size) {
		raise(format("String too big: % > %", size, max_size));
		return {};
	}

	string out(size, ' ');
	loadData(out);
	if(!m_is_valid)
		out = {};
	return out;
}

int MemoryStream::loadString(Span<char> str) {
	PASSERT(str.size() >= 1);
	auto size = loadSize();
	int max_size = str.size() - 1;
	if(size > max_size) {
		raise(format("String too big: % > %", size, max_size));
		str[0] = 0;
		return 0;
	}

	loadData(span(str.data(), size));
	if(!m_is_valid)
		size = 0;
	str[size] = 0;
	return size;
}

PodVector<char> MemoryStream::loadVector(int max_size, int element_size) {
	PASSERT(max_size >= 0 && element_size >= 1);
	auto size = loadSize();

	if(size > max_size) {
		raise(format("Vector too big: % > %", size, max_size));
		return {};
	}
	auto byte_size = size * element_size;
	ASSERT(byte_size < INT_MAX);

	PodVector<char> out(byte_size);
	loadData(out);
	if(!m_is_valid)
		out.clear();
	return out;
}

void MemoryStream::signature(u32 sig) {
	if(!m_is_loading) {
		*this << sig;
		return;
	}

	if(!m_is_valid)
		return;
	u32 tmp;
	*this >> tmp;

	if(tmp != sig) {
		char sigc[12] = {
			char((sig >> 0) & 0xff),  0, 0, char((sig >> 8) & 0xff),  0, 0,
			char((sig >> 16) & 0xff), 0, 0, char((sig >> 24) & 0xff), 0, 0,
		};

		for(int k = 0; k < 4; k++) {
			if(sigc[k * 3] == 0) {
				sigc[k * 3 + 0] = '\\';
				sigc[k * 3 + 1] = '0';
			}
		}

		raise(stdFormat("Expected signature: 0x%08x (\"%s%s%s%s\")", sig, sigc + 0, sigc + 3,
						sigc + 6, sigc + 9));
	}
}

void MemoryStream::signature(Str str) {
	constexpr int max_signature_size = 32;
	DASSERT(str.size() <= max_signature_size);

	if(!m_is_loading) {
		saveData(str);
		return;
	}
	if(!m_is_valid)
		return;

	char buf[max_signature_size + 1];
	loadData(span(buf, str.size()));

	if(memcmp(buf, str.data(), str.size()) != 0) {
		char rsig[256], dsig[256];
		decodeString(str, rsig);
		decodeString(Str(buf, str.size()), dsig);
		raise(format("Expected signature: \"%\" got: \"%\"", rsig, dsig));
	}
}

MemoryStream memoryLoader(CSpan<char> data) { return MemoryStream(data); }
MemoryStream memoryLoader(vector<char> vec) {
	PodVector<char> pvec;
	pvec.unsafeSwap(vec);
	return MemoryStream(move(pvec), true);
}
MemoryStream memoryLoader(PodVector<char> vec) { return MemoryStream(move(vec), true); }

MemoryStream memorySaver(Span<char> buf) { return MemoryStream(buf); }
MemoryStream memorySaver(int capacity) { return MemoryStream(PodVector<char>(capacity), false); }
MemoryStream memorySaver(PodVector<char> buffer) { return MemoryStream(move(buffer), false); }
