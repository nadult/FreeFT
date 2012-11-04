#ifndef ENTITY_H
#define ENTITY_H

#include "base.h"

namespace gfx { class SceneRenderer; }

class Entity {
public:
	Entity(int3 bbox, int3 pos);
	virtual ~Entity() { }

	virtual void addToRender(gfx::SceneRenderer&) const { }

	void fixPos();
	void setPos(float3);
	IBox boundingBox() const;

protected:
	float3 m_pos;
	int3 m_bbox;
};

#endif
