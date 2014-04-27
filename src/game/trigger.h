/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_TRIGGER_H
#define GAME_TRIGGER_H

#include "game/entity.h"

namespace game {

	DECLARE_ENUM(TriggerClassId,
		generic,
		spawn_zone
	);

	class Trigger: public Entity {
	public:
		enum { type_id = EntityId::trigger };

		Trigger(TriggerClassId::Type class_id, const FBox &box);
		Trigger(const XMLNode&);
		Trigger(Stream&);
		
		void save(Stream&) const override;
		XMLNode save(XMLNode &parent) const override;
		
		Entity *clone() const override;

		Flags::Type flags() const override { return Flags::trigger; }
		EntityId::Type typeId() const override { return EntityId::trigger; }
		const FBox boundingBox() const override;
		
		const IRect screenRect() const override;
		const IRect currentScreenRect() const override { return screenRect(); }

		void setBox(const FBox &box);

		TriggerClassId::Type classId() const { return m_class_id; }

	private:
		TriggerClassId::Type m_class_id;
		float3 m_box_size;
	};

}

#endif
