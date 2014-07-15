/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "base.h"
#include <cstdlib>
#include <ext/vstring.h>
#include <cstdarg>

const char* strcasestr(const char *a, const char *b) {
	DASSERT(a && b);

	while(*a) {
		if(strcasecmp(a, b) == 0)
			return a;
		a++;
	}

	return nullptr;
}

/*int strcasecmp(const char *a, const char *b) {
	DASSERT(a && b);
	while(*a && tolower(*a) == tolower(*b)) {
		a++;
		b++;
	}

	return *a < *b? -1 : *a == *b? 0 : 1;
}*/

const Color Color::white		(255, 255, 255);
const Color Color::gray			(127, 127, 127);
const Color Color::yellow		(255, 255, 0);
const Color Color::red			(255, 0, 0);
const Color Color::green		(0, 255, 0);
const Color Color::blue			(0, 0, 255);
const Color Color::black		(0, 0, 0);
const Color Color::transparent	(0, 0, 0, 0);

Color mulAlpha(Color color, float alpha_mul) {
	return Color(float4(color) * float4(1.0f, 1.0f, 1.0f, alpha_mul));
}

Color lerp(Color a, Color b, float value) {
	return Color(lerp((float4)a, (float4)b, value));
}

Color desaturate(Color col, float value) {
	float4 rgba(col);
	float avg = sqrtf(rgba.x * rgba.x * 0.299f + rgba.y * rgba.y * 0.587f + rgba.z * rgba.z * 0.114f);
	return lerp(col, Color(avg, avg, avg, rgba.w), value);
}

bool toBool(const char *input) {
	CString str(input);
	if(caseEqual(str, "true"))
		return true;
	if(caseEqual(str, "false"))
		return false;
	return toInt(input) != 0;
}

int toInt(const char *str) {
	DASSERT(str);

	if(!str[0])
		return 0;

	int out;
	if(sscanf(str, "%d", &out) != 1)
		THROW("Error while converting string \"%s\" to int", str);
	return out;
}

static void toFloat(const char *input, int count, float *out) {
	DASSERT(input);
	if(!input[0]) {
		for(int i = 0; i < count; i++)
			out[i] = 0.0f;
		return;
	}
	if(sscanf(input, "%f %f %f %f" + (4 - count) * 3, out + 0, out + 1, out + 2, out + 3) != count)
		THROW("Error while converting string \"%s\" to float%d", input, count);
}

float toFloat(const char *input) {
	float out;
	toFloat(input, 1, &out);
	return out;
}

float2 toFloat2(const char *input) {
	float2 out;
	toFloat(input, 2, &out.x);
	return out;
}

float3 toFloat3(const char *input) {
	float3 out;
	toFloat(input, 3, &out.x);
	return out;
}


float4 toFloat4(const char *input) {
	float4 out;
	toFloat(input, 4, &out.x);
	return out;
}

uint toFlags(const char *input, const char **strings, int num_strings, uint first_flag) {
	const char *iptr = input;

	uint out_value = 0;
	while(*iptr) {
		const char *next_space = strchr(iptr, ' ');
		int len = next_space? next_space - iptr : strlen(iptr);

		bool found = false;
		for(int e = 0; e < num_strings; e++)
			if(strncmp(iptr, strings[e], len) == 0 && strings[e][len] == 0) {
				out_value |= first_flag << e;
				found = true;
				break;
			}

		if(!found) {
			char flags[1024], *ptr = flags;
			for(int i = 0; i < num_strings; i++)
				ptr += snprintf(ptr, sizeof(flags) - (ptr - flags), "%s ", strings[i]);
			if(num_strings)
				ptr[-1] = 0;

			THROW("Error while converting string \"%s\" to flags (%s)", input, flags);
		}

		if(!next_space)
			break;
		iptr = next_space + 1;
	}

	return out_value;
}

int fromString(const char *str, const char **strings, int count) {
	DASSERT(str);
	for(int n = 0; n < count; n++)
		if(strcmp(str, strings[n]) == 0)
			return n;
	
	char tstrings[1024], *ptr = tstrings;
	for(int i = 0; i < count; i++)
		ptr += snprintf(ptr, sizeof(tstrings) - (ptr - tstrings), "%s ", strings[i]);
	if(count)
		ptr[-1] = 0;

	THROW("Error while finding string \"%s\" in array (%s)", str, tstrings);
	return -1;
}

BitVector::BitVector(int size) :m_data((size + base_size - 1) / base_size), m_size(size) { }

void BitVector::resize(int new_size, bool clear_value) {
	PodArray<u32> new_data(new_size);
	memcpy(new_data.data(), m_data.data(), sizeof(base_type) * min(new_size, m_data.size()));
	if(new_data.size() > m_data.size())
		memset(new_data.data() + m_data.size(), clear_value? 0xff : 0, (new_data.size() - m_data.size()) * sizeof(base_type));
	m_data.swap(new_data);
}
	
void BitVector::clear(bool value) {
	memset(m_data.data(), value? 0xff : 0, m_data.size() * sizeof(base_type));
}

TextFormatter::TextFormatter(int size) :m_data(size), m_offset(0) {
	DASSERT(size > 0);
	m_data[0] = 0;
}

void TextFormatter::operator()(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	m_offset += vsnprintf(&m_data[m_offset], m_data.size() - m_offset, format, ap);
	va_end(ap);
}

const string format(const char *format, ...) {
	char buffer[4096];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	return std::move(string(buffer));
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

/*

#include "../libs/lz4/lz4.h"
#include "../libs/lz4/lz4hc.h"

enum { max_size = 16 * 1024 * 1024 };

//TODO: testme
void compress(const PodArray<char> &in, PodArray<char> &out, bool hc) {
	ASSERT(in.size() <= max_size);

	PodArray<char> temp(LZ4_compressBound(in.size()));
	int size = (hc? LZ4_compressHC : LZ4_compress)(in.data(), temp.data(), in.size());
	//int size = in.size(); memcpy(temp.data(), in.data(), size);

	out.resize(size + 4);
	memcpy(out.data(), &size, 4);
	memcpy(out.data() + 4, temp.data(), size);
}

void decompress(const PodArray<char> &in, PodArray<char> &out) {
	i32 size;
	ASSERT(in.size() >= 4);
	memcpy(&size, in.data(), 4);
	ASSERT(size <= max_size);
	out.resize(size);

//	memcpy(out.data(), in.data() + 4, size);
//	int decompressed_bytes = size;
	int decompressed_bytes = LZ4_uncompress(in.data() + 4, out.data(), size);
	ASSERT(decompressed_bytes == size);
}

*/
