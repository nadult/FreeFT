// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_TURRET_H
#define GAME_TURRET_H

#include "game/thinking_entity.h"
#include <fwk/enum_map.h>

namespace game {

	DEFINE_ENUM(TurretAction,
		idle,
		attack_single,
		attack_burst,
		hiding,
		showing,
		hidden,

		death,
		death_electrify,
		death_explode
	);

	DEFINE_ENUM(TurretSoundId,
		death,
		death_explode,
		attack_single,
		attack_burst,
		arming,
		unarming
	);

	struct TurretProto: public ProtoImpl<TurretProto, EntityProto, ProtoId::turret> {
		TurretProto(const TupleParser&);

		bool canHide() const;

		enum { invalid_anim_id = 255 };
		EnumMap<TurretAction, u8> anim_idx;
		EnumMap<TurretSoundId, SoundId> sound_idx;
		float hit_points;
	};

	class Turret: public EntityImpl<Turret, TurretProto, EntityId::turret, ThinkingEntity> {
	public:
		Turret(Stream&);
		Turret(const XMLNode&);
		Turret(const Proto &proto);

		FlagsType flags() const override;
		const FBox boundingBox() const override;

		bool setOrder(POrder&&, bool force = false) override;
		void onImpact(DamageType, float damage, const float3 &force, EntityRef source) override;

		XMLNode save(XMLNode&) const override;
		void save(Stream&) const override;

		TurretAction action() const { return m_action; }

		bool canSee(EntityRef ref, bool simple_test = false) override;
		bool isDying() const override;
		bool isDead() const override;

		int hitPoints() const { return m_hit_points; }
		DeathId deathType(DamageType, float damage, const float3 &force) const;

	private:
		void think() override;
		void nextFrame() override;

		void updateArmour();

		void lookAt(const float3 &pos, bool at_once = false);
		
		void fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness = 0.0f);

		bool animateDeath(DeathId);
		bool animate(TurretAction);

		//TODO: overloaded virtuals warnings in Makefile
		bool handleOrder(IdleOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(LookAtOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(AttackOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(DieOrder&, EntityEvent, const EntityEventParams&) override;


	private:
		float m_target_angle;
		TurretAction m_action;
		int m_hit_points;
	};

}

#endif
