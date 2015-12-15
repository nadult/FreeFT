/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/trigger.h"

namespace game {

	DEFINE_ENUM(TriggerClassId,
		"generic",
		"spawn_zone"
	);

	Trigger::Trigger(TriggerClassId::Type class_id, const FBox &box)
		:Entity(Sprite::getDummy()), m_class_id(class_id), m_faction_id(0), m_spawn_delay(0.0f), m_spawn_limit(0) {
		setBox(box);
	}
		
	Trigger::Trigger(const XMLNode &node) :Entity(Sprite::getDummy(), node) {
		m_class_id = TriggerClassId::fromString(node.attrib("class"));
		m_box_size = node.attrib<float3>("box_size");
		m_faction_id = node.attrib<int>("faction_id", 0);
		m_spawn_delay = node.attrib<float>("spawn_delay", 0.0f);
		m_spawn_limit = node.attrib<int>("spawn_limit", 0);
	}

	Trigger::Trigger(Stream &sr) :Entity(Sprite::getDummy(), sr) {
		sr >> m_box_size >> m_class_id >> m_faction_id >> m_spawn_delay >> m_spawn_limit;
	}
		
	void Trigger::save(Stream &sr) const {
		Entity::save(sr);
		sr << m_box_size << m_class_id << m_faction_id << m_spawn_delay << m_spawn_limit;
	}

	XMLNode Trigger::save(XMLNode &parent) const {
		XMLNode out = Entity::save(parent);
		out.addAttrib("box_size", m_box_size);
		out.addAttrib("class", TriggerClassId::toString(m_class_id));
		if(m_faction_id != 0)
			out.addAttrib("faction_id", m_faction_id);
		if(m_spawn_delay != 0.0f)
			out.addAttrib("spawn_delay", m_spawn_delay);
		if(m_spawn_limit != 0.0f)
			out.addAttrib("spawn_limit", m_spawn_limit);
		return out;
	}

	Entity *Trigger::clone() const {
		return new Trigger(*this);
	}

	void Trigger::setBox(const FBox &box) {
		setPos(box.min);
		m_box_size = box.size();
	}
		
	const FBox Trigger::boundingBox() const {
		return FBox(pos(), pos() + m_box_size);
	}
		
	const IRect Trigger::screenRect() const {
		return IRect(worldToScreen(boundingBox()));
	}

}
