// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"
#include "sys/data_sheet.h"

namespace game {
	
	DEFINE_ENUM(ImpactType,
		ranged,
		melee,
		area,
		area_safe // Safe for spawner
	);

	struct ImpactProto: public ProtoImpl<ImpactProto, EntityProto, ProtoId::impact> {
		ImpactProto(const TupleParser &parser);

		SoundId sound_idx;
		DamageType damage_type;
		float damage, force, range;
		ImpactType type;
		bool is_invisible;
	};

	class Impact: public EntityImpl<Impact, ImpactProto, EntityId::impact> {
	public:
		Impact(const ImpactProto&, EntityRef source, EntityRef target, float damage_mod);
		Impact(Stream&);

		void save(Stream&) const override;
		XmlNode save(XmlNode parent) const override;
		void addToRender(SceneRenderer &out, Color color) const override;

		FlagsType flags() const override { return Flags::impact | Flags::dynamic_entity; }
	
	protected:
		void onAnimFinished() override;
		void think() override;
		void applyDamage();

		EntityRef m_target, m_source;
		float m_damage_mod;
		bool m_played_sound;
		bool m_applied_damage;
	};

}
