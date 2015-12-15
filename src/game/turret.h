/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_TURRET_H
#define GAME_TURRET_H

#include "game/thinking_entity.h"

namespace game {

	DECLARE_ENUM(TurretAction,
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

	DECLARE_ENUM(TurretSoundId,
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
		u8 anim_idx[TurretAction::count];
		SoundId sound_idx[TurretSoundId::count];
		float hit_points;
	};

	class Turret: public EntityImpl<Turret, TurretProto, EntityId::turret, ThinkingEntity> {
	public:
		Turret(Stream&);
		Turret(const XMLNode&);
		Turret(const Proto &proto);

		Flags::Type flags() const override;
		const FBox boundingBox() const override;

		bool setOrder(POrder&&, bool force = false) override;
		void onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) override;

		XMLNode save(XMLNode&) const override;
		void save(Stream&) const override;

		TurretAction::Type action() const { return m_action; }

		bool canSee(EntityRef ref, bool simple_test = false) override;
		bool isDying() const override;
		bool isDead() const override;

		int hitPoints() const { return m_hit_points; }
		DeathId::Type deathType(DamageType::Type, float damage, const float3 &force) const;

	private:
		void think() override;
		void nextFrame() override;

		void updateArmour();

		void lookAt(const float3 &pos, bool at_once = false);
		
		void fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness = 0.0f);

		bool animateDeath(DeathId::Type);
		bool animate(TurretAction::Type);

		//TODO: overloaded virtuals warnings in Makefile
		bool handleOrder(IdleOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(LookAtOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(AttackOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(DieOrder&, EntityEvent::Type, const EntityEventParams&) override;


	private:
		float m_target_angle;
		TurretAction::Type m_action;
		int m_hit_points;
	};

}

#endif
