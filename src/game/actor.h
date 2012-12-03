#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/entity.h"

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

	namespace WeaponClassId {
		enum Type {
			unarmed,
			club,
			heavy,
			knife,
			minigun,
			pistol,
			rifle,
			rocket,
			smg,
			spear,
			
			count,
		};
	};

	namespace OrderId {
		enum Type {
			do_nothing,
			move,
			attack,
			change_stance,
			change_weapon,
			interact,

			count,
		};
	}

	struct Order {
		Order(OrderId::Type id = OrderId::do_nothing) :id(id) { }
		
		struct Move			{ int3 target_pos; bool run; };
		struct ChangeStance	{ int next_stance; };
		struct Attack		{ int3 target_pos; int mode; };
		struct ChangeWeapon { WeaponClassId::Type target_weapon; };
		struct Interact {
			Entity *target; //TODO: pointer may become invalid
			bool waiting_for_move;
		};

		OrderId::Type id;
		union {
			Move move;
			Attack attack;
			ChangeStance change_stance;
			ChangeWeapon change_weapon;
			Interact interact;
		};
	};
		
	Order moveOrder(int3 target_pos, bool run);
	Order doNothingOrder();
	Order changeStanceOrder(int next_stance);
	Order attackOrder(int attack_mode, const int3 &target_pos);
	Order changeWeaponOrder(WeaponClassId::Type target_weapon);
	Order interactOrder(Entity *target);

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
		Actor(const char *spr_name, const int3 &pos);

		void setNextOrder(const Order &order);
		WeaponClassId::Type weaponId() const { return m_weapon_id; }
		
		virtual bool isStatic() const { return false; }

	protected:
		void think();

		void issueNextOrder();
		void issueMoveOrder();

		void setWeapon(WeaponClassId::Type);
		void setSequence(ActionId::Type);
		void lookAt(int3 pos);

		void animate(int);
		void onAnimFinished();

		// orders
		bool m_issue_next_order;
		Order m_order;
		Order m_next_order;

		// movement state
		int m_target_dir;
		int3 m_last_pos;
		float m_path_t;
		int m_path_pos;
		vector<int3> m_path;

		ActionId::Type m_action_id;
		StanceId::Type m_stance_id;
		WeaponClassId::Type m_weapon_id;

		ActorAnimMap m_anim_map;
	};

}

#endif
