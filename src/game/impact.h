/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_IMPACT_H
#define GAME_IMPACT_H

#include "game/entity.h"
#include "sys/data_sheet.h"

namespace game {
	
	DECLARE_ENUM(ImpactType,
		ranged,
		melee,
		area
	);

	struct ImpactProto: public ProtoImpl<ImpactProto, EntityProto, ProtoId::impact> {
		ImpactProto(const TupleParser &parser);

		SoundId sound_idx;
		DamageType::Type damage_type;
		float damage, force, range;
		ImpactType::Type type;
		bool is_invisible;
	};

	class Impact: public EntityImpl<Impact, ImpactProto, EntityId::impact> {
	public:
		Impact(const ImpactProto&, EntityRef source, EntityRef target, float damage_mod);
		Impact(Stream&);

		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		Flags::Type flags() const { return Flags::impact | Flags::dynamic_entity; }
	
	protected:
		void onAnimFinished() override;
		void think() override;

		EntityRef m_target, m_source;
		float m_damage_mod;

		//TODO: Ugly hack; Add possibility to create entity with world as a parameter,
		//this way we will be able to play sounds that should be played on the first frame
		bool m_played_sound;
		bool m_applied_damage;
	};

}

#endif
