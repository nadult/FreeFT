// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"

namespace game {

	DEFINE_ENUM(TriggerClassId,
		generic,
		spawn_zone
	);

	class Trigger: public Entity {
	public:
		enum { type_id = (int)EntityId::trigger };

		Trigger(TriggerClassId class_id, const FBox &box);
		Trigger(CXmlNode);
		Trigger(Stream&);
		
		void save(Stream&) const override;
		XmlNode save(XmlNode parent) const override;
		
		Entity *clone() const override;

		FlagsType flags() const override { return Flags::trigger; }
		EntityId typeId() const override { return EntityId::trigger; }
		const FBox boundingBox() const override;
		
		const IRect screenRect() const override;
		const IRect currentScreenRect() const override { return screenRect(); }

		void setBox(const FBox &box);

		TriggerClassId classId() const { return m_class_id; }

		void setFactionId(int faction_id) { m_faction_id = faction_id; }
		int factionId() const { return m_faction_id; }

		void setSpawnDelay(float spawn_delay) { m_spawn_delay = spawn_delay; }
		float spawnDelay() const { return m_spawn_delay; }
		
		void setSpawnLimit(int spawn_limit) { m_spawn_limit = spawn_limit; }
		int spawnLimit() const { return m_spawn_limit; }

	private:
		TriggerClassId m_class_id;
		int m_faction_id, m_spawn_limit;
		float m_spawn_delay;
		float3 m_box_size;
	};

}
