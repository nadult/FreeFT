/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_PACKED_TEXTURE_H
#define GFX_PACKED_TEXTURE_H

#include "base.h"

// Alpha component is always 255
class Palette {
  public:
	void load(Stream &);
	void save(Stream &) const;
	void legacyLoad(Stream &);

	void resize(int size);
	void clear();
	int size() const { return m_data.size(); }
	bool isEmpty() const { return m_data.isEmpty(); }
	bool operator==(const Palette &) const;

	void set(int idx, Color col) { m_data[idx] = Color(col, 255); }
	Color operator[](int idx) const { return m_data[idx]; }
	const Color *data() const { return m_data.data(); }

  protected:
	PodArray<Color> m_data;
};

// Pallettized, RLE - encoded (as in ZAR) texture
class PackedTexture {
  public:
	PackedTexture();

	void legacyLoad(Stream &, Palette &);
	void load(Stream &);
	void save(Stream &) const;

	int width() const { return m_width; }
	int height() const { return m_height; }
	int2 size() const { return int2(m_width, m_height); }
	int memorySize() const;

	void decode(Color *__restrict out_data, const Color *__restrict pal, int pal_size) const;
	void toTexture(Texture &, const Color *pal, int pal_size) const;
	void blit(Texture &, const int2 &offset, const Color *palette, int size) const;
	bool testPixel(const int2 &pixel) const;

  protected:
	PodArray<u8> m_data;
	int m_width, m_height;
	u8 m_default_idx, m_max_idx;
};

#endif
