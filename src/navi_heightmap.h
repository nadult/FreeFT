/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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
