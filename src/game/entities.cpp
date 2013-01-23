/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */


#include "game/entity.h"
#include "game/actor.h"
#include "game/item.h"
#include "game/door.h"
#include "game/container.h"
#include <sys/xml.h>

namespace game {

	Entity *Entity::constructFromXML(const XMLNode &node) {
		EntityId::Type entity_type = EntityId::fromString(node.name());

		float3 pos = node.float3Attrib("pos");
		float angle = node.floatAttrib("angle");
		const char *sprite_name = node.attrib("sprite");

		Entity *out = nullptr;

		if(entity_type == EntityId::actor) {
			ActorTypeId::Type type = ActorTypeId::fromString(node.attrib("actor_type"));
			out = new Actor(type, pos);
		}
		else if(entity_type == EntityId::door) {
			DoorTypeId::Type type = DoorTypeId::fromString(node.attrib("door_type"));
			out = new Door(sprite_name, pos, type, rotateVector(float2(1, 0), angle));
		}
		else if(entity_type == EntityId::container) {
			out = new Container(sprite_name, pos);
		}
		else if(entity_type == EntityId::item) {
			out = new ItemEntity(ItemDesc::find(node.attrib("item_desc")), pos);
		}

		out->setDirAngle(angle);
			
		if(!out)
			THROW("Unknown entity type: %d\n", (int)entity_type);

		return out;
	}
	
	void Entity::saveToXML(XMLNode &parent) const {
		XMLNode node = parent.addChild(EntityId::toString(entityType()));
		node.addAttrib("pos", m_pos);
		node.addAttrib("sprite", node.own(m_sprite->resourceName()));
		node.addAttrib("angle", m_dir_angle);
		saveContentsToXML(node);
	}
	
	void Actor::saveContentsToXML(XMLNode &node) const {
		node.addAttrib("actor_type", ActorTypeId::toString(m_type_id));
	}
	
	void Container::saveContentsToXML(XMLNode &node) const {
		
	}
	
	void Door::saveContentsToXML(XMLNode &node) const {
		node.addAttrib("door_type", DoorTypeId::toString(m_type));
	}
	
	void ItemEntity::saveContentsToXML(XMLNode &node) const {
		node.addAttrib("item_desc", node.own(m_item.desc()->name.c_str()));
	}

}
