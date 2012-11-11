#include "entity.h"

Entity::Entity(int3 bbox, int3 pos) :m_tile_map(nullptr), m_navigation_map(nullptr), m_bbox(bbox), m_pos(0, 0, 0) {
	setPos(pos);
}

void Entity::fixPos() {
	m_pos = (int3)(m_pos + float3(0.5f, 0, 0.5f));
}

void Entity::setPos(float3 new_pos) {
	DASSERT(new_pos.x >= 0.0f && new_pos.y >= 0.0f && new_pos.z >= 0.0f);
	m_pos = new_pos;
}

IBox Entity::boundingBox() const {
	int3 ipos(m_pos);
	float eps = 0.0001f;
	int3 frac(m_pos.x - ipos.x > eps?1 : 0, m_pos.y - ipos.y > eps? 1 : 0, m_pos.z - ipos.z > eps? 1 : 0);
	return IBox(ipos, ipos + m_bbox + frac);
}
