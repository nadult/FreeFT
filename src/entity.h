#ifndef ENTITY_H
#define ENTITY_H

#include "base.h"

namespace gfx { class SceneRenderer; }

class TileMap;

class Entity {
public:
	Entity(int3 bbox, int3 pos);
	virtual ~Entity() { }

	virtual void addToRender(gfx::SceneRenderer&) const { }

	void fixPos();
	void setPos(float3);
	float3 pos() const { return m_pos; }
	IBox boundingBox() const;

	//TODO: pointer to Scene albo World context or something
	TileMap *m_tile_map;

protected:
	float3 m_pos;
	int3 m_bbox;
};

#endif
