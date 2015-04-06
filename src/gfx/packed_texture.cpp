/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/packed_texture.h"
#include <cstring>

namespace {

void loadZAR(Stream &sr, PodArray<Color> &out_data, int2 &out_size) {
	PackedTexture packed;
	Palette palette;
	packed.legacyLoad(sr, palette);
	out_data.resize(packed.width() * packed.height());
	packed.decode(out_data.data(), palette.data(), palette.size());
	out_size = packed.size();
}

Texture::RegisterLoader zar_loader("zar", loadZAR);
}

bool Palette::operator==(const Palette &rhs) const {
	return size() == rhs.size() && memcmp(data(), rhs.data(), size()) == 0;
}

void Palette::resize(int size) {
	DASSERT(size <= 256);
	m_data.resize(size);
	for(int n = 0; n < size; n++)
		m_data[n] = Color(0, 0, 0, 255);
}

void Palette::clear() { m_data.clear(); }

void Palette::legacyLoad(Stream &sr) {
	i32 pal_size;
	sr >> pal_size;
	ASSERT(pal_size <= 256);
	resize(pal_size);

	Color buf[256];
	sr.loadData(buf, pal_size * sizeof(Color));

	for(int n = 0; n < pal_size; n++)
		set(n, swapBR(buf[n]));
}

void Palette::load(Stream &sr) {
	u8 rgb[256 * 3], *ptr = rgb;
	u16 size;
	sr >> size;

	m_data.resize(size);
	sr.loadData(rgb, size * 3);

	for(int n = 0; n < size; n++) {
		m_data[n] = Color(ptr[0], ptr[1], ptr[2]);
		ptr += 3;
	}
}

void Palette::save(Stream &sr) const {
	u8 rgb[256 * 3], *ptr = rgb;
	sr << u16(m_data.size());

	for(int n = 0; n < m_data.size(); n++) {
		ptr[0] = m_data[n].r;
		ptr[1] = m_data[n].g;
		ptr[2] = m_data[n].b;
		ptr += 3;
	}

	sr.saveData(rgb, m_data.size() * 3);
}

PackedTexture::PackedTexture() : m_width(0), m_height(0), m_default_idx(0), m_max_idx(0) {}

void PackedTexture::legacyLoad(Stream &sr, Palette &palette) {
	sr.signature("<zar>", 6);

	char zar_type, dummy1, has_palette;

	sr.unpack(zar_type, dummy1, m_width, m_height, has_palette);

	if(zar_type != 0x33 && zar_type != 0x34)
		THROW("Wrong zar type: %d", (int)zar_type);

	if(has_palette)
		palette.legacyLoad(sr);
	else
		palette.clear();

	u32 value;
	sr >> value;

	m_default_idx = 0;
	if((zar_type == 0x34 || zar_type == 0x33) && has_palette)
		m_default_idx = value & 255;
	m_max_idx = m_default_idx;

	int offset = 0, total_pixels = m_width * m_height;
	vector<char> tdata;

	while(offset < total_pixels) {
		u8 buf[128];
		u8 cmd;
		sr >> cmd;
		int n_pixels = cmd >> 2;
		int command = cmd & 3;
		tdata.push_back(cmd);

		if(command == 1) {
			sr.loadData(buf, n_pixels);
			tdata.insert(tdata.end(), buf, buf + n_pixels);
			for(int n = 0; n < n_pixels; n++)
				m_max_idx = max(m_max_idx, buf[n]);
		} else if(command == 2) {
			sr.loadData(buf, n_pixels * 2);
			tdata.insert(tdata.end(), buf, buf + n_pixels * 2);
			for(int n = 0; n < n_pixels; n++)
				m_max_idx = max(m_max_idx, buf[n * 2 + 0]);
		} else if(command == 3) {
			sr.loadData(buf, n_pixels);
			tdata.insert(tdata.end(), buf, buf + n_pixels);
		}
		offset += n_pixels;
	}

	m_data.resize(tdata.size());
	memcpy(m_data.data(), tdata.data(), m_data.size());
}

void PackedTexture::load(Stream &sr) {
	sr.unpack(m_width, m_height, m_default_idx, m_max_idx);
	sr >> m_data;
}

void PackedTexture::save(Stream &sr) const {
	sr.pack(m_width, m_height, m_default_idx, m_max_idx);
	sr << m_data;
}

int PackedTexture::memorySize() const { return sizeof(PackedTexture) + m_data.size(); }

static const Color blend(Color dst, Color src) __attribute((always_inline));
static const Color blend(Color dst, Color src) {
	int dr = dst.r, dg = dst.g, db = dst.b, da = dst.a;
	int sr = src.r, sg = src.g, sb = src.b, sa = src.a;

	return Color(((sr - dr) * sa + (dr << 8)) >> 8, ((sg - dg) * sa + (dg << 8)) >> 8,
				 ((sb - db) * sa + (db << 8)) >> 8, ((256 - da) * sa + (da << 8)) >> 8);
}

void PackedTexture::blit(Texture &out, const int2 &pos, const Color *__restrict pal,
						 int pal_size) const {
	DASSERT(pos.x + m_width <= out.width() && pos.y + m_height <= out.height());
	DASSERT(pos.x >= 0 && pos.y >= 0);

	Color *dst = out.line(pos.y) + pos.x;
	int pixels = m_width * m_height, stride = out.width() - m_width;

	DASSERT(pal && m_max_idx < pal_size);
	const u8 *data = m_data.data();
	Color default_col = pal[m_default_idx];

	int line = m_width;
	while(data < m_data.end()) {
		int n_pixels = *data >> 2;
		int command = *data++ & 3;

#define LOOP(...)                                                                                  \
	while(n_pixels) {                                                                              \
		int to_copy = min(n_pixels, line);                                                         \
		__VA_ARGS__ line -= to_copy;                                                               \
		dst += to_copy;                                                                            \
		n_pixels -= to_copy;                                                                       \
		if(!line) {                                                                                \
			line = m_width;                                                                        \
			dst += stride;                                                                         \
		}                                                                                          \
	}

		if(__builtin_expect(command == 0, true)) {
			LOOP()
		} else if(command == 1) {
			LOOP(for(int i = 0; i < to_copy; i++) dst[i] = Color(pal[data[i]], 255);
				 data += to_copy;)
		} else if(command == 2) {
			LOOP(for(int i = 0; i < to_copy; i++) {
					 Color col(pal[data[i * 2 + 0]], data[i * 2 + 1]);
					 dst[i] = dst[i].a ? blend(dst[i], col) : col;
				 } data += to_copy * 2;)
		} else if(command == 3) {
			LOOP(for(int i = 0; i < to_copy; i++) dst[i] =
					 blend(dst[i], Color(default_col, data[i]));
				 data += to_copy;)
		}
	}

#undef LOOP
}

void PackedTexture::decode(Color *__restrict dst, const Color *__restrict pal, int pal_size) const {
	DASSERT(pal && m_max_idx < pal_size);
	const u8 *data = m_data.data();
	Color default_col = pal[m_default_idx];

	while(data < m_data.end()) {
		int n_pixels = *data >> 2;
		int command = *data++ & 3;

		if(__builtin_expect(command == 0, true)) {
			memset(dst, 0, n_pixels * sizeof(Color));
		} else if(command == 1) {
			for(int i = 0; i < n_pixels; i++)
				dst[i] = pal[data[i]];
			data += n_pixels;
		} else if(command == 2) {
			for(int i = 0; i < n_pixels; i++)
				dst[i] = Color(pal[data[i * 2 + 0]], data[i * 2 + 1]);
			data += n_pixels * 2;
		} else {
			for(int i = 0; i < n_pixels; i++)
				dst[i] = Color(default_col, data[i]);
			data += n_pixels;
		}

		dst += n_pixels;
	}
}

void PackedTexture::toTexture(Texture &out, const Color *pal, int pal_size) const {
	out.resize(m_width, m_height);
	decode(out.line(0), pal, pal_size);
}

bool PackedTexture::testPixel(const int2 &pixel) const {
	if(pixel.x < 0 || pixel.y < 0 || pixel.x >= m_width || pixel.y >= m_height)
		return false;

	const u8 *data = m_data.data();
	int target_offset = pixel.x + pixel.y * m_width;
	int offset = 0;

	while(data < m_data.end()) {
		int n_pixels = *data >> 2;
		int command = *data++ & 3;
		offset += n_pixels;

		if(offset > target_offset) {
			offset = target_offset - (offset - n_pixels);
			return command == 0 ? false : command == 1
											  ? true
											  : data[command == 2 ? offset * 2 + 1 : offset] != 0;
		}

		if(command >= 1)
			data += command == 2 ? n_pixels * 2 : n_pixels;
	}

	return false;
}
