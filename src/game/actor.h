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

			count,
		};
	}

	enum InteractionMode {
		interact_normal,
		interact_pickup,
		interact_use_item,
	};

	struct Order {
		Order(OrderId::Type id = OrderId::do_nothing) :id(id) { }
		
		struct Move			{ int3 target_pos; bool run; };
		struct ChangeStance	{ int next_stance; };
		struct Attack		{ int3 target_pos; int mode; };
		struct Interact {
			Entity *target; //TODO: pointer may become invalid
			InteractionMode mode;
			bool waiting_for_move;
		};

		OrderId::Type id;
		union {
			Move move;
			Attack attack;
			ChangeStance change_stance;
			Interact interact;
		};
	};
		
	Order moveOrder(int3 target_pos, bool run);
	Order doNothingOrder();
	Order changeStanceOrder(int next_stance);
	Order attackOrder(int attack_mode, const int3 &target_pos);
	Order interactOrder(Entity *target, InteractionMode mode);
	Order dropItemOrder();

	class ActorInventory: public Inventory {
	public:
		void add(const Item &item);
		Item drop();

		Item armour, weapon, ammo;
	};

	class ActorAnimMap {
	public:
		ActorAnimMap(gfx::PSprite);
		ActorAnimMap() = default;

		int sequenceId(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;
		string sequenceName(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;
		//TODO: verification that some basic sequences are available?

	private:
		vector<int> m_seq_ids;
	};

	class Actor: public Entity {
	public:
		Actor(const char *spr_name, const float3 &pos);
		virtual ColliderFlags colliderType() const { return collider_dynamic; }
		virtual EntityFlags entityType() const { return entity_actor; }

		void setNextOrder(const Order &order);
		const WeaponDesc *currentWeapon() const;

	protected:
		void think();

		void issueNextOrder();
		void issueMoveOrder();

		void updateWeapon();
		void setSequence(ActionId::Type);
		void lookAt(const float3 &pos, bool at_once = false);

		void nextFrame();
		void onAnimFinished();

		void onFireEvent(const int3&);
		void onSoundEvent();
		void onPickupEvent();

	private:
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

		WeaponClassId::Type m_weapon_class_id;
		ActionId::Type m_action_id;
		StanceId::Type m_stance_id;

		ActorInventory m_inventory;

		ActorAnimMap m_anim_map;
	};

}

#endif
