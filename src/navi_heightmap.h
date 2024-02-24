// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

class NaviHeightmap {
  public:
	static constexpr int invalid_value = 0;

	NaviHeightmap(const int2 &size);

	void update(const vector<IBox> &walkable, const vector<IBox> &blockers);

	int2 dimensions() const { return m_size; }

	int levelCount() const { return m_level_count; }
	int index(int x, int y, int level) const { return x + (level * m_size.y + y) * m_size.x; }

	u8 operator[](int idx) const { return m_data[idx]; }
	u8 &operator[](int idx) { return m_data[idx]; }
	u8 operator()(int x, int y, int level) const { return m_data[index(x, y, level)]; }

	bool test(int x, int y, int level, int agent_size) const;

	Image toImage(int level) const;
	void saveLevels() const;
	void printInfo() const;

  private:
	void addLevel();

	vector<u8> m_data;
	int m_level_count;
	int2 m_size;
};
