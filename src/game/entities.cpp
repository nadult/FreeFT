// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/actor.h"
#include "game/container.h"
#include "game/door.h"
#include "game/entity.h"
#include "game/item.h"
#include "game/projectile.h"
#include "game/trigger.h"
#include "game/turret.h"

namespace game {

Entity *Entity::construct(CXmlNode node) {
	auto entity_type = fromString<EntityId>(node.name());
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
		FATAL("Unknown entity id: %d\n", (int)entity_type);

	return out;
}

//TODO: properly saving enums (so that they take less space)
//TODO: endianess
Entity *Entity::construct(MemoryStream &sr) {
	EntityId entity_type;
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
		FATAL("Unknown entity id: %d\n", (int)entity_type);

	return out;
}

}
