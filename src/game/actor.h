// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/inventory.h"
#include "game/path.h"
#include "game/thinking_entity.h"
#include <fwk/enum_map.h>

namespace game {

	// Three types of actions:
	// normal which have different animations for different weapon types
	// simple which have same animations for different weapon types
	// special handled separately by different functions
	namespace Action {
		enum Type : char {
			_normal, // Normal anims: (stance x weapon)
			idle = _normal,
			walk,
			breathe,

			_simple, // Simple anims: (stance)
			fall_back = _simple,
			fallen_back,
			fall_forward,
			fallen_forward,
			getup_back,
			getup_forward,
			recoil,
			dodge1,
			dodge2,
			fidget,
			magic,
			magic_low,
			run,
			stance_up,
			stance_down,
			pickup,

			_special, // Special anims:
			attack = _special,
			climb,
			climb_up,
			climb_down,
			death,

			count
		};

		bool isNormal(int);
		bool isSimple(int);
		bool isSpecial(int);
		bool isValid(int);
	}

	struct ActorProto;

	struct ActorArmourProto
		: public ProtoImpl<ActorArmourProto, EntityProto, ProtoId::actor_armour> {
		ActorArmourProto(const TupleParser &, bool is_actor = false);

		void link();

		int climbAnimId(Action::Type);
		int attackAnimId(AttackMode, Stance, WeaponClass) const;
		int deathAnimId(DeathId) const;
		int simpleAnimId(Action::Type, Stance) const;
		int animId(Action::Type, Stance, WeaponClass) const;

		bool canChangeStance() const;
		bool canEquipWeapon(WeaponClass) const;
		bool canUseWeapon(WeaponClass, Stance) const;

		ProtoRef<ArmourProto> armour;
		ProtoRef<ActorProto> actor;

		EnumMap<Stance, EnumMap<SurfaceId, SoundId>> step_sounds;
		SoundId fall_sound;

	private:
		void initAnims();

		u8 m_climb_idx[3];
		EnumMap<DeathId, u8> m_death_idx;
		EnumMap<Stance, u8> m_simple_idx[Action::_special - Action::_simple];
		EnumMap<Stance, EnumMap<WeaponClass, u8>> m_normal_idx[Action::_simple  - Action::_normal];
		EnumMap<WeaponClass, EnumMap<AttackMode, EnumMap<Stance, u8>>> m_attack_idx;
		EnumMap<WeaponClass, bool> m_can_equip_weapon;
		EnumMap<WeaponClass, EnumMap<Stance, bool>> m_can_use_weapon;
		bool m_can_change_stance;
		bool m_is_actor;
	};

	struct ActorProto: public ProtoImpl<ActorProto, ActorArmourProto, ProtoId::actor> {
		ActorProto(const TupleParser&);

		void link();

		ProtoRef<WeaponProto> punch_weapon;
		ProtoRef<WeaponProto> kick_weapon;
		string sound_prefix;
		bool is_heavy;
		bool is_alive;

		// prone, crouch, walk, run
		float speeds[count<Stance>() + 1];

		float hit_points;

		struct Sounds {
			EnumMap<DeathId, SoundId> death;
			SoundId hit;
			SoundId get_up;
		};

		// Different sound variations; At least one is always present
		vector<Sounds> sounds;
		EnumMap<DeathId, SoundId> human_death_sounds;
		SoundId run_sound, walk_sound;
	};

	class Actor: public EntityImpl<Actor, ActorArmourProto, EntityId::actor, ThinkingEntity> {
	public:
		Actor(Stream&);
		Actor(CXmlNode);
		Actor(const Proto &proto, Stance stance = Stance::stand);
		Actor(const Actor &rhs, const Proto &new_proto);
		Actor(const Proto &proto, ActorInventory &inventory);

		FlagsType flags() const override;
		const FBox boundingBox() const override;

		using ThinkingEntity::setOrder;
		bool setOrder(POrder &&, bool force = false) override;
		void onImpact(DamageType, float damage, const float3 &force, EntityRef source) override;

		XmlNode save(XmlNode) const override;
		void save(Stream &) const override;

		SurfaceId surfaceUnder() const;
		WeaponClass equippedWeaponClass() const;

		Stance stance() const { return m_stance; }
		Action::Type action() const { return m_action; }

		const ActorInventory &inventory() const { return m_inventory; }

		bool canSee(EntityRef ref, bool simple_test = false) override;
		bool canEquipItem(const Item &item) const;
		bool canChangeStance() const;
		bool isDying() const override;
		bool isDead() const override;

		int hitPoints() const { return m_hit_points; }

		int factionId() const override { return m_faction_id; }
		int clientId() const { return m_client_id; }
		void setFactionId(int faction_id) { m_faction_id = faction_id; }
		void setClientId(int client_id) { m_client_id = client_id; }

		const Path currentPath() const;
		const float3 estimateMove(float time_advance) const;
		void followPath(const Path &path, PathPos &pos);
		void fixPosition();

		float dodgeChance(DamageType, float damage) const;
		float fallChance(DamageType, float damage, const float3 &force) const;
		float interruptChance(DamageType, float damage, const float3 &force) const;
		DeathId deathType(DamageType, float damage, const float3 &force) const;

		Maybe<AttackMode> validateAttackMode(Maybe<AttackMode>) const;

		const FBox shootingBox(const Weapon &weapon) const override;
		float accuracy(const Weapon &weapon) const override;

	  private:
		void think() override;
		void nextFrame() override;

		void updateArmour();

		void lookAt(const float3 &pos, bool at_once = false);

		void addToRender(SceneRenderer &out, Color color) const override;

		void fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness = 0.0f);
		void makeImpact(EntityRef target, const Weapon &weapon);

		bool animateDeath(DeathId);
		bool animate(Action::Type);

		bool handleOrder(IdleOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(LookAtOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(MoveOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(TrackOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(AttackOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(ChangeStanceOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(InteractOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(DropItemOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(EquipItemOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(UnequipItemOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(TransferItemOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(GetHitOrder&, EntityEvent, const EntityEventParams&) override;
		bool handleOrder(DieOrder&, EntityEvent, const EntityEventParams&) override;


	private:
		bool shrinkRenderedBBox() const override { return true; }

		enum class FollowPathResult {
			moved,
			finished,
			collided,
		};

		FollowPathResult followPath(const Path&, PathPos&, bool run);

		const ActorProto &m_actor;

		float m_target_angle;
		Stance m_stance;
		Action::Type m_action;
		int m_faction_id, m_client_id;
		int m_sound_variation;

		ActorInventory m_inventory;

		int m_hit_points;
		
		float3 m_move_vec;
	};

}
