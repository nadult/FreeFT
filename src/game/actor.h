/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/thinking_entity.h"
#include "game/inventory.h"
#include "game/path.h"

namespace game {

	// Three types of actions:
	// normal which have different animations for different weapon types
	// simple which have same animations for different weapon types
	// special handled separately by different functions
	namespace Action {
		enum Type: char {
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

	struct ActorArmourProto: public ProtoImpl<ActorArmourProto, EntityProto, ProtoId::actor_armour> {
		ActorArmourProto(const TupleParser&, bool is_actor = false);

		void link();

		int climbAnimId(Action::Type);
		int attackAnimId(AttackMode::Type, Stance::Type, WeaponClass::Type) const;
		int deathAnimId(DeathId::Type) const;
		int simpleAnimId(Action::Type, Stance::Type) const;
		int animId(Action::Type, Stance::Type, WeaponClass::Type) const;

		bool canChangeStance() const;
		bool canEquipWeapon(WeaponClass::Type) const;
		bool canUseWeapon(WeaponClass::Type, Stance::Type) const;

		ProtoRef<ArmourProto> armour;
		ProtoRef<ActorProto> actor;

		SoundId step_sounds[Stance::count][SurfaceId::count];
		SoundId fall_sound;

	private:
		void initAnims();

		u8 m_climb_idx[3];
		u8 m_death_idx[DeathId::count];
		u8 m_simple_idx[Action::_special - Action::_simple][Stance::count];
		u8 m_normal_idx[Action::_simple  - Action::_normal][Stance::count][WeaponClass::count];
		u8 m_attack_idx[WeaponClass::count][AttackMode::count][Stance::count];
		bool m_can_equip_weapon[WeaponClass::count];
		bool m_can_use_weapon[WeaponClass::count][Stance::count];
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
		float speeds[Stance::count + 1];

		float hit_points;

		struct Sounds {
			SoundId death[DeathId::count];
			SoundId hit;
			SoundId get_up;
		};

		// Different sound variations; At least one is always present
		vector<Sounds> sounds;
		SoundId human_death_sounds[DeathId::count];
		SoundId run_sound, walk_sound;
	};

	class Actor: public EntityImpl<Actor, ActorArmourProto, EntityId::actor, ThinkingEntity> {
	public:
		Actor(Stream&);
		Actor(const XMLNode&);
		Actor(const Proto &proto, Stance::Type stance = Stance::stand);
		Actor(const Actor &rhs, const Proto &new_proto);
		Actor(const Proto &proto, ActorInventory &inventory);

		Flags::Type flags() const;
		const FBox boundingBox() const override;

		bool setOrder(POrder&&, bool force = false) override;
		void onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) override;

		XMLNode save(XMLNode&) const;
		void save(Stream&) const;

		SurfaceId::Type surfaceUnder() const;
		WeaponClass::Type equippedWeaponClass() const;

		Stance::Type stance() const { return m_stance; }
		Action::Type action() const { return m_action; }

		const ActorInventory &inventory() const { return m_inventory; }
		
		bool canSee(EntityRef ref, bool simple_test = false) override;
		bool canEquipItem(const Item &item) const;
		bool canChangeStance() const;
		bool isDying() const override;
		bool isDead() const override;

		int hitPoints() const { return m_hit_points; }

		int factionId() const { return m_faction_id; }
		int clientId() const { return m_client_id; }
		void setFactionId(int faction_id) { m_faction_id = faction_id; }
		void setClientId(int client_id) { m_client_id = client_id; }

		const Path currentPath() const;
		const float3 estimateMove(float time_advance) const;
		void followPath(const Path &path, PathPos &pos);
		void fixPosition();

		float dodgeChance(DamageType::Type, float damage) const;
		float fallChance(DamageType::Type, float damage, const float3 &force) const;
		float interruptChance(DamageType::Type, float damage, const float3 &force) const;
		DeathId::Type deathType(DamageType::Type, float damage, const float3 &force) const;

		AttackMode::Type validateAttackMode(AttackMode::Type mode) const;
	
		const FBox shootingBox(const Weapon &weapon) const override;
		float accuracy(const Weapon &weapon) const override;

	private:
		void think() override;
		void nextFrame() override;

		void updateArmour();

		void lookAt(const float3 &pos, bool at_once = false);
		
		void addToRender(gfx::SceneRenderer &out, Color color) const override;

		void fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness = 0.0f);
		void makeImpact(EntityRef target, const Weapon &weapon);

		bool animateDeath(DeathId::Type);
		bool animate(Action::Type);

		bool handleOrder(IdleOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(LookAtOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(MoveOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(TrackOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(AttackOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(ChangeStanceOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(InteractOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(DropItemOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(EquipItemOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(UnequipItemOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(TransferItemOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(GetHitOrder&, EntityEvent::Type, const EntityEventParams&) override;
		bool handleOrder(DieOrder&, EntityEvent::Type, const EntityEventParams&) override;


	private:
		bool shrinkRenderedBBox() const { return true; }

		enum class FollowPathResult {
			moved,
			finished,
			collided,
		};

		FollowPathResult followPath(const Path&, PathPos&, bool run);

		const ActorProto &m_actor;

		float m_target_angle;
		Stance::Type m_stance;
		Action::Type m_action;
		int m_faction_id, m_client_id;
		int m_sound_variation;

		ActorInventory m_inventory;

		int m_hit_points;
		
		float3 m_move_vec;
	};


}

#endif
