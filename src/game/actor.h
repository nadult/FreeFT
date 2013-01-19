/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/entity.h"
#include "game/inventory.h"

namespace game {

	namespace StanceId {
		enum Type {
			standing,
			crouching,
			prone,

			count,
		};
	}
	namespace ActionId {
		enum Type {
			standing,
			walking,
			running,

			stance_up,
			stance_down,

			attack1,
			attack2,

			pickup,
			magic1,
			magic2,

			count,
		};

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

	class ActorAnimMap {
	public:
		ActorAnimMap(PSprite);
		ActorAnimMap() = default;

		int sequenceId(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;
		string sequenceName(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;
		//TODO: verification that some basic sequences are available?

	private:
		vector<int> m_seq_ids;
	};

	class Actor: public Entity {
	public:
		Actor(ActorTypeId::Type, const float3 &pos);
		virtual ColliderFlags colliderType() const { return collider_dynamic; }
		virtual EntityId::Type entityType() const { return EntityId::actor; }

		void setNextOrder(const Order &order);
		const ActorInventory &inventory() const { return m_inventory; }

	protected:
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

		void setSequence(ActionId::Type);
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

		const ActorTypeId::Type m_type_id;
		WeaponClassId::Type m_weapon_class_id;
		ArmourClassId::Type m_armour_class_id;

		ActionId::Type m_action_id;
		StanceId::Type m_stance_id;

		ActorInventory m_inventory;

		ActorAnimMap m_anim_map;
	};

}

#endif
