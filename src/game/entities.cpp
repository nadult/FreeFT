/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/entity.h"
#include "game/actor.h"
#include "game/item.h"
#include "game/door.h"
#include "game/container.h"
#include <sys/xml.h>

namespace game {

	Entity* Entity::construct(const XMLNode &node) {
		EntityId::Type entity_type = EntityId::fromString(node.name());

		float3 pos = node.float3Attrib("pos");
		float angle = node.floatAttrib("angle");
		const char *sprite_name = node.attrib("sprite");

		//TODO: exception safety
		Entity *out = nullptr;

		if(entity_type == EntityId::actor)
			out = new Actor(node);
		else if(entity_type == EntityId::door)
			out = new Door(node);
		else if(entity_type == EntityId::container)
			out = new Container(node);
		else if(entity_type == EntityId::item)
			out = new ItemEntity(node);
			
		if(!out)
			THROW("Unknown entity type: %d\n", (int)entity_type);

		return out;
	}

	//TODO: properly saving enums (so that they take less space)
	//TODO: endianess
	Entity* Entity::construct(Stream &sr) {
		EntityId::Type entity_type;
		sr >> entity_type;

		Entity *out = nullptr;

		if(entity_type == EntityId::actor)
			out = new Actor(sr);
		else if(entity_type == EntityId::door)
			out = new Door(sr);
		else if(entity_type == EntityId::container)
			out = new Container(sr);
		else if(entity_type == EntityId::item)
			out = new ItemEntity(sr);

		if(!out)
			THROW("Unknown entity type: %d\n", (int)entity_type);

		return out;
	}


}
