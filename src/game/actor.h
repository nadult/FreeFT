/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/entity.h"
#include "game/inventory.h"
#include "game/weapon.h"
#include "game/orders.h"

namespace game {

	// Two types of actions:
	// normal which have different animations for different weapon types
	// simple which have same animations for different weapon types
	//
	// TODO: additional actions: breathe, fall, dodge, getup, recoil?
	
	//TODO: deaths with overlays

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

		void connect();

		int climbAnimId(Action::Type);
		int attackAnimId(AttackMode::Type, Stance::Type, WeaponClass::Type) const;
		int deathAnimId(DeathTypeId::Type) const;
		int simpleAnimId(Action::Type, Stance::Type) const;
		int animId(Action::Type, Stance::Type, WeaponClass::Type) const;

		bool canChangeStance() const;
		bool canEquipWeapon(WeaponClass::Type) const;
		bool canUseWeapon(WeaponClass::Type, Stance::Type) const;

		ProtoRef<ArmourProto> armour;
		ProtoRef<ActorProto> actor;

		SoundId step_sounds[Stance::count][SurfaceId::count];

	private:
		void initAnims();

		u8 m_climb_idx[3];
		u8 m_death_idx[DeathTypeId::count];
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

		void connect();

		string sound_prefix;
		bool is_heavy;
		bool is_alive;

		// prone, crouch, walk, run
		float speeds[Stance::count + 1];
		
		//TODO: add sound variations, each actor instance will have different sound set
		SoundId death_sounds[DeathTypeId::count];
		SoundId human_death_sounds[DeathTypeId::count];
	};

	class Actor: public EntityImpl<Actor, ActorArmourProto, EntityId::actor> {
	public:
		Actor(Stream&);
		Actor(const XMLNode&);
		Actor(const Proto &proto, Stance::Type stance = Stance::stand);
		Actor(const Actor &rhs, const Proto &new_proto);

		ColliderFlags colliderType() const { return collider_dynamic; }

		bool setOrder(POrder&&);
		void onImpact(DeathTypeId::Type, float damage);

		XMLNode save(XMLNode&) const;
		void save(Stream&) const;

		SurfaceId::Type surfaceUnder() const;
		WeaponClass::Type equippedWeaponClass() const;
		OrderTypeId::Type currentOrder() const;

		Stance::Type stance() const { return m_stance; }
		Action::Type action() const { return m_action; }
		const ActorInventory &inventory() const { return m_inventory; }
		
		bool canEquipItem(int item_id) const;
		bool canChangeStance() const;
		bool isDead() const;

	private:
		void think();

		void updateArmour();

		void lookAt(const float3 &pos, bool at_once = false);

		void nextFrame();
		void onAnimFinished();

		void onFireEvent(const int3&);
		void onSoundEvent();
		void onStepEvent(bool left_foot);
		void onPickupEvent();

		void fireProjectile(const int3 &offset, const float3 &target, const Weapon &weapon,
								float random_val = 0.0f);

		bool animateDeath(DeathTypeId::Type);
		bool animate(Action::Type);
		
	private:
		bool shrinkRenderedBBox() const { return true; }

		typedef void (Actor::*HandleFunc)(Order*, ActorEvent::Type, const ActorEventParams&);

		void updateOrderFunc();
		void handleOrder(ActorEvent::Type, const ActorEventParams &params = ActorEventParams{});

		template <class TOrder>
		void handleOrder(Order *order, ActorEvent::Type event, const ActorEventParams &params) {
			DASSERT(order && order->typeId() == (OrderTypeId::Type)TOrder::type_id);
			if((TOrder::event_flags & event) && !order->isFinished())
				if(!handleOrder(*static_cast<TOrder*>(order), event, params))
					order->finish();
		}
		void emptyHandleFunc(Order*, ActorEvent::Type, const ActorEventParams&) { }
		
		bool handleOrder(IdleOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(LookAtOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(MoveOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(AttackOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(ChangeStanceOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(InteractOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(DropItemOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(EquipItemOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(UnequipItemOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(TransferItemOrder&, ActorEvent::Type, const ActorEventParams&);
		bool handleOrder(DieOrder&, ActorEvent::Type, const ActorEventParams&);

		const ActorProto &m_actor;

		POrder m_order;
		vector<POrder> m_following_orders;
		HandleFunc m_order_func;

		float m_target_angle;
		Stance::Type m_stance;
		Action::Type m_action;

		ActorInventory m_inventory;
	};


}

#endif
