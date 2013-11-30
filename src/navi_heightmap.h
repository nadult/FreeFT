/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NAVI_HEIGHTMAP_H
#define NAVI_HEIGHTMAP_H

#include "base.h"

class NaviHeightmap {
public:
	enum {
		invalid_value = 0
	};

	NaviHeightmap(const int2 &size);

	//TODO: support for non-walkable objects
	void update(const vector<IBox> &boxes);

	int2 dimensions() const { return m_size; }

	int levelCount() const { return m_level_count; }
	int index(int x, int y, int level) const { return x + (level * m_size.y + y) * m_size.x; }

	u8 operator[](int idx) const { return m_data[idx]; }
	u8 &operator[](int idx) { return m_data[idx]; }
	u8 operator()(int x, int y, int level) const { return m_data[index(x, y, level)]; }

	bool test(int x, int y, int level, int agent_size) const;

	const gfx::Texture toTexture(int level) const;
	void saveLevels() const;
	void printInfo() const;

private:
	void addLevel();

	vector<u8> m_data;
	int m_level_count;
	int2 m_size;
};



#endif
