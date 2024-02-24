// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"
#include "game/tile_map.h"
#include "grid.h"
#include "occluder_map.h"

namespace game {

// Grid based container for entities
class EntityMap : public Grid {
  public:
	typedef Grid::TObjectDef<Entity> ObjectDef;

	explicit EntityMap(const TileMap &, const int2 &dimensions = int2(0, 0));
	~EntityMap();
	EntityMap(const EntityMap &) = delete;
	void operator=(const EntityMap &) = delete;

	const ObjectDef &operator[](int idx) const {
		return reinterpret_cast<const ObjectDef &>(Grid::operator[](idx));
	}
	ObjectDef &operator[](int idx) { return reinterpret_cast<ObjectDef &>(Grid::operator[](idx)); }

	void resize(const int2 &new_dims);

	int add(Dynamic<Entity> &&ptr, int index = -1);
	void update(int index);
	void remove(int index);

	int pixelIntersect(const int2 &pos, FlagsType flags = Flags::all) const;

	Ex<void> loadFromXML(const XmlDocument &);
	void saveToXML(XmlDocument &) const;
	void updateVisibility(const OccluderConfig &);

  protected:
	void updateOccluderId(int object_id);

	const TileMap &m_tile_map;
};

}
