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
	namespace ActionId {
		enum Type: char {
			first_normal,
			idle = first_normal,
			walking,
			attack1,
			attack2,

			first_simple,
			running = first_simple,
			stance_up,
			stance_down,
			pickup,
			magic1,
			magic2,

			first_special,
			death = first_special,

			count,
		};

		bool isNormal(Type);
		bool isSimple(Type);
		bool isSpecial(Type);
	}


	struct ActorProto;

	struct ActorArmourProto: public ProtoImpl<ActorArmourProto, EntityProto, ProtoId::actor_armour> {
		ActorArmourProto(const TupleParser&, bool is_actor = false);

		void connect();

		const string deathAnimName(DeathTypeId::Type) const;
		const string simpleAnimName(ActionId::Type, Stance::Type) const;
		const string animName(ActionId::Type, Stance::Type, WeaponClassId::Type) const;

		//TODO: methods for additional checking
		//TODO: checking if animation is valid in these methods:
		int deathAnimId(DeathTypeId::Type) const;
		int simpleAnimId(ActionId::Type, Stance::Type) const;
		int animId(ActionId::Type, Stance::Type, WeaponClassId::Type) const;

		bool canChangeStance() const;

		// When some animation is not-available, it will be changed
		// to default
		void setFallbackAnims();

		ProtoRef<ArmourProto> armour;
		ProtoRef<ActorProto> actor;

		SoundId step_sounds[Stance::count][SurfaceId::count];

	private:
		void initAnims();

		short m_death_ids[DeathTypeId::count];
		short m_simple_ids[ActionId::first_special - ActionId::first_simple][Stance::count];
		short m_normal_ids[ActionId::first_simple  - ActionId::first_normal][Stance::count][WeaponClassId::count];
		bool is_actor;
	};

	struct ActorProto: public ProtoImpl<ActorProto, ActorArmourProto, ProtoId::actor> {
		ActorProto(const TupleParser&);

		bool is_heavy;

		// prone, crouch, walk, run
		float speeds[Stance::count + 1];
	};

	class Actor: public EntityImpl<Actor, ActorArmourProto, EntityId::actor> {
	public:
		Actor(Stream&);
		Actor(const XMLNode&);
		Actor(const Proto &proto, Stance::Type stance = Stance::standing);
		Actor(const Actor &rhs, const Proto &new_proto);

		virtual ColliderFlags colliderType() const { return collider_dynamic; }

		bool setOrder(POrder&&);
		const ActorInventory &inventory() const { return m_inventory; }
		void onImpact(int projectile_type, float damage);

		bool isDead() const;
		
		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;

		SurfaceId::Type surfaceUnder() const;
		Stance::Type stance() const { return m_stance; }

		OrderTypeId::Type currentOrder() const { return m_order? m_order->typeId() : OrderTypeId::invalid; }

	private:
		void initialize();

		void think();

		//TODO: orders are getting too complicated, refactor them
		void issueNextOrder();
		void issueMoveOrder();

		void updateArmour();

		bool canEquipItem(int item_id) const;

		// TODO: Some weapons can be equipped, but cannot be fired in every possible stance
		// What sux even more: some weapons can be fired only when actor has the armour on
		// (some sprites have some animations missing...)!
		bool canEquipWeapon(WeaponClassId::Type) const;
		bool canChangeStance() const;

		void animate(ActionId::Type);
		void animateDeath(DeathTypeId::Type);
		void lookAt(const float3 &pos, bool at_once = false);

		void nextFrame();
		void onAnimFinished();

		void onFireEvent(const int3&);
		void onSoundEvent();
		void onStepEvent(bool left_foot);
		void onPickupEvent();

		void fireProjectile(const int3 &offset, const float3 &target, const Weapon &weapon,
								float random_val = 0.0f);
		
	private:
		virtual bool shrinkRenderedBBox() const { return true; }

		typedef void (Actor::*HandleFunc)(Order*, ActorEvent::Type, const ActorEventParams&);

		void updateOrderFunc();
		void handleOrder(ActorEvent::Type, const ActorEventParams &params = ActorEventParams{});

		template <class TOrder>
		void handleOrder(Order *order, ActorEvent::Type event, const ActorEventParams &params) {
			DASSERT(order && order->typeId() == (OrderTypeId::Type)TOrder::type_id);
			if((TOrder::event_flags & event) && !order->isFinished())
				handleOrder(*static_cast<TOrder*>(order), event, params);
		}
		void emptyHandleFunc(Order*, ActorEvent::Type, const ActorEventParams&) { }
		
		void handleOrder(IdleOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(MoveOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(AttackOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(ChangeStanceOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(InteractOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(DropItemOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(EquipItemOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(UnequipItemOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(TransferItemOrder&, ActorEvent::Type, const ActorEventParams&);
		void handleOrder(DieOrder&, ActorEvent::Type, const ActorEventParams&);

		const ActorProto &m_actor;

		POrder m_order;
		POrder m_next_order;
		HandleFunc m_order_func;

		float m_target_angle;
		ActionId::Type m_action_id;
		Stance::Type m_stance;

		ActorInventory m_inventory;
	};


}

#endif
