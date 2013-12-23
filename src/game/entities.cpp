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
			const ItemDesc *desc = ItemDesc::find(node.attrib("item_desc"));
			if(!desc)
				THROW("Unknown item id: %s\n", node.attrib("item_desc"));
			out = new ItemEntity(desc, pos);
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
		node.addAttrib("door_type", DoorTypeId::toString(m_type_id));
	}
	
	void ItemEntity::saveContentsToXML(XMLNode &node) const {
		node.addAttrib("item_desc", node.own(m_item.desc()->id.c_str()));
	}
	
	Entity::Entity(Stream &sr) {
		float3 pos;
		float angle;
		char sprite_name[256];

		sr.unpack(pos, angle);
		sr.loadString(sprite_name, sizeof(sprite_name));
		initialize(sprite_name, pos);
		m_dir_angle = angle;
	}

	//TODO: properly saving enums (so that they take less space)
	//TODO: endianess
	Entity *Entity::constructFromBinary(Stream &sr) {
		EntityId::Type entity_type;
		sr << entity_type;

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

	void Entity::saveToBinary(Stream &sr) {
		ASSERT(sr.isSaving());

		EntityId::Type entity_type = entityType();
		sr << entity_type;
		sr.pack(m_pos, m_dir_angle);
		sr.saveString(m_sprite->resourceName());
	}

}
