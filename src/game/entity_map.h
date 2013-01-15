#ifndef ENTITY_MAP_H
#define ENTITY_MAP_H

#include "grid.h"
#include "occluder_map.h"
#include "game/entity.h"

namespace game
{

	class EntityMap: public Grid
	{
	public:
		typedef Grid::TObjectDef<Entity> ObjectDef;
	
		const ObjectDef &operator [](int idx) const
			{ return reinterpret_cast<const ObjectDef&>(Grid::operator[](idx)); }

		EntityMap(const int2 &dimensions = int2(0, 0));
		void resize(const int2 &new_dims);
	
		void add(Entity*);
		void remove(const Entity*);
		void update(const Entity*);
		
		int pixelIntersect(const int2 &pos, int flags = collider_flags) const;
	};

}

#endif
