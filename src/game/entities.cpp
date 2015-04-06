/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */


#include "game/entity.h"
#include "game/actor.h"
#include "game/turret.h"
#include "game/item.h"
#include "game/door.h"
#include "game/container.h"
#include "game/projectile.h"
#include "game/trigger.h"

namespace game {

	Entity* Entity::construct(const XMLNode &node) {
		EntityId::Type entity_type = EntityId::fromString(node.name());
		Entity *out = nullptr;

		if(entity_type == EntityId::actor)
			out = new Actor(node);
		else if(entity_type == EntityId::turret)
			out = new Turret(node);
		else if(entity_type == EntityId::door)
			out = new Door(node);
		else if(entity_type == EntityId::container)
			out = new Container(node);
		else if(entity_type == EntityId::item)
			out = new ItemEntity(node);
		else if(entity_type == EntityId::trigger)
			out = new Trigger(node);
			
		if(!out)
			THROW("Unknown entity id: %s\n", (int)entity_type);

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
		else if(entity_type == EntityId::turret)
			out = new Turret(sr);
		else if(entity_type == EntityId::door)
			out = new Door(sr);
		else if(entity_type == EntityId::container)
			out = new Container(sr);
		else if(entity_type == EntityId::item)
			out = new ItemEntity(sr);
		else if(entity_type == EntityId::projectile)
			out = new Projectile(sr);
		else if(entity_type == EntityId::impact)
			out = new Impact(sr);
		else if(entity_type == EntityId::trigger)
			out = new Trigger(sr);

		if(!out)
			THROW("Unknown entity id: %d\n", (int)entity_type);

		return out;
	}


}
