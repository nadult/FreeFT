/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/entity.h"
#include "game/inventory.h"

namespace game {

	// Two types of actions:
	// normal which have different animations for different weapon types
	// simple which have same animations for different weapon types
	//
	// TODO: additional actions: breathe, fall, dodge, getup, recoil?
	namespace ActionId {
		enum Type {
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

	namespace OrderId {
		enum Type {
			do_nothing,
			move,
			attack,
			change_stance,
			interact,
			drop_item,
			equip_item,
			unequip_item,
			transfer_item,
			die,

			count,
		};
	}

	enum InteractionMode {
		interact_normal,
		interact_pickup,
		interact_use_item,
	};

	enum TransferMode {
		transfer_to,
		transfer_from,
	};

	struct Order {
		Order(OrderId::Type id = OrderId::do_nothing) :id(id) { }
		
		struct Die			{ DeathTypeId::Type death_type; };
		struct Move			{ int3 target_pos; bool run; };
		struct ChangeStance	{ int next_stance; };
		struct Attack		{ int3 target_pos; int mode; };
		struct Interact {
			InteractionMode mode;
			bool waiting_for_move;
		};
		struct DropItem { int item_id; };
		struct EquipItem { int item_id; };
		struct UnequipItem { InventorySlotId::Type slot_id; };
		struct TransferItem { int item_id, count; TransferMode mode; };

		OrderId::Type id;
		EntityRef target;
		union {
			Die die;
			Move move;
			Attack attack;
			ChangeStance change_stance;
			Interact interact;
			DropItem drop_item;
			EquipItem equip_item;
			UnequipItem unequip_item;
			TransferItem transfer_item;
		};
	};
	
	Order dieOrder(DeathTypeId::Type);	
	Order moveOrder(int3 target_pos, bool run);
	Order doNothingOrder();
	Order changeStanceOrder(int next_stance);
	Order attackOrder(int attack_mode, const int3 &target_pos);
	Order interactOrder(Entity *target, InteractionMode mode);

	//TODO: zamiast idkow, w rozkazach przekazywac cale obiekty? jesli, np.
	//w trakcje wykonywania rozkazu zmieni sie stan kontenera, to zostanie
	//przekazany nie ten item co trzeba
	Order dropItemOrder(int item_id);
	Order transferItemOrder(Entity *target, TransferMode mode, int item_id, int count);
	Order equipItemOrder(int item_id);
	Order unequipItemOrder(InventorySlotId::Type item_id);

	//TODO: this should be shared among actors with the same sprites
	class ActorAnims {
	public:
		ActorAnims(PSprite);
		ActorAnims() = default;

		const string deathAnimName(DeathTypeId::Type) const;
		const string simpleAnimName(ActionId::Type, StanceId::Type) const;
		const string animName(ActionId::Type, StanceId::Type, WeaponClassId::Type) const;

		//TODO: methods for additional checking
		//TODO: checking if animation is valid in these methods:
		int deathAnimId(DeathTypeId::Type) const;
		int simpleAnimId(ActionId::Type, StanceId::Type) const;
		int animId(ActionId::Type, StanceId::Type, WeaponClassId::Type) const;

		bool canChangeStance() const;

		// When some animation is not-available, it will be changed
		// to default
		void setFallbackAnims();

	private:
		short m_death_ids[DeathTypeId::count];
		short m_simple_ids[ActionId::first_special - ActionId::first_simple][StanceId::count];
		short m_normal_ids[ActionId::first_simple  - ActionId::first_normal][StanceId::count][WeaponClassId::count];
	};

	class Actor: public Entity {
	public:
		Actor(Stream&);
		Actor(const XMLNode&);
		Actor(ActorTypeId::Type type, const float3 &pos);

		virtual ColliderFlags colliderType() const { return collider_dynamic; }
		virtual EntityId::Type entityType() const { return EntityId::actor; }
		virtual Entity *clone() const;
		ActorTypeId::Type actorType() const { return m_type_id; }

		void setNextOrder(const Order &order);
		const ActorInventory &inventory() const { return m_inventory; }
		void onImpact(ProjectileTypeId::Type projectile_type, float damage);

		bool isDead() const;
		
		virtual void save(XMLNode&) const;
		virtual void save(Stream&) const;

	private:
		void initialize(ActorTypeId::Type);

		void think();

		//TODO: orders are getting too complicated, refactor them
		void issueNextOrder();
		void issueMoveOrder();

		void updateWeapon();
		void updateArmour();

		bool canEquipItem(int item_id) const;

		// TODO: Some weapons can be equipped, but cannot be fired in every possible stance
		// What sux even more: some weapons can be fired only when actor has the armour on
		// (some sprites have some animations missing...)!
		bool canEquipWeapon(WeaponClassId::Type) const;
		bool canEquipArmour(ArmourClassId::Type) const;
		bool canChangeStance() const;

		void animate(ActionId::Type);
		void animateDeath(DeathTypeId::Type);
		void lookAt(const float3 &pos, bool at_once = false);

		void nextFrame();
		void onAnimFinished();

		void onFireEvent(const int3&);
		void onSoundEvent();
		void onPickupEvent();
		
	private:
		virtual bool shrinkRenderedBBox() const { return true; }
		// orders
		bool m_issue_next_order;
		Order m_order;
		Order m_next_order;

		// movement state
		float m_target_angle;
		int3 m_last_pos;
		float m_path_t;
		int m_path_pos;
		vector<int3> m_path;

		ActorTypeId::Type m_type_id;
		WeaponClassId::Type m_weapon_class_id;
		ArmourClassId::Type m_armour_class_id;

		ActionId::Type m_action_id;
		StanceId::Type m_stance_id;

		ActorInventory m_inventory;

		ActorAnims m_anims;
	};

}

#endif
