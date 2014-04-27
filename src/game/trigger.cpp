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
		:Entity(Sprite::getDummy()), m_class_id(class_id) {
		setBox(box);
	}
		
	Trigger::Trigger(const XMLNode &node) :Entity(Sprite::getDummy(), node) {
		m_class_id = TriggerClassId::fromString(node.attrib("class"));
		m_box_size = node.float3Attrib("box_size");
	}

	Trigger::Trigger(Stream &sr) :Entity(Sprite::getDummy(), sr) {
		sr >> m_box_size >> m_class_id;
	}
		
	void Trigger::save(Stream &sr) const {
		Entity::save(sr);
		sr << m_box_size << m_class_id;
	}

	XMLNode Trigger::save(XMLNode &parent) const {
		XMLNode out = Entity::save(parent);
		out.addAttrib("box_size", m_box_size);
		out.addAttrib("class", TriggerClassId::toString(m_class_id));
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
