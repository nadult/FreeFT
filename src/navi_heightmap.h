#ifndef NAVI_HEIGHTMAP_H
#define NAVI_HEIGHTMAP_H

#include "base.h"

class NaviHeightmap {
public:
	NaviHeightmap(const int2 &size);
	void update(const vector<IBox> &boxes);

	int2 dimensions() const { return m_size; }

	int levelCount() const { return m_level_count; }
	int index(int x, int y, int level) const { return x + (level * m_size.y + y) * m_size.x; }

	short operator[](int idx) const { return m_data[idx]; }
	short &operator[](int idx) { return m_data[idx]; }

	bool test(int x, int y, int level, int extents) const;

	const gfx::Texture toTexture(int level) const;
	void saveLevels() const;
	void printInfo() const;

private:
	void addLevel();

	vector<short> m_data;
	int m_level_count;
	int2 m_size;
};



#endif
