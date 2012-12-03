#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "game/entity.h"
#include "gfx/sprite.h"


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

		count,
	};

	bool isLooped(Type);
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

		count,
	};
}

struct Order {
	Order(OrderId::Type id = OrderId::do_nothing) :id(id) { }
	
	struct Move			{ int3 target_pos; bool run; };
	struct ChangeStance	{ int next_stance; };
	struct Attack		{ int3 target_pos; int mode; };
	struct ChangeWeapon { WeaponClassId::Type target_weapon; };

	OrderId::Type id;
	union {
		Move move;
		Attack attack;
		ChangeStance change_stance;
		ChangeWeapon change_weapon;
	};
};
	
Order moveOrder(int3 target_pos, bool run);
Order doNothingOrder();
Order changeStanceOrder(int next_stance);
Order attackOrder(int attack_mode, const int3 &target_pos);
Order changeWeaponOrder(WeaponClassId::Type target_weapon);

class AnimationMap {
public:
	AnimationMap(gfx::PSprite);
	AnimationMap() = default;

	int sequenceId(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;
	string sequenceName(StanceId::Type, ActionId::Type, WeaponClassId::Type) const;

private:
	vector<int> m_seq_ids;
};

class Actor: public Entity {
public:
	Actor(const char *spr_name, int3 pos);

	void setNextOrder(const Order &order);
	void think(double current_time, double time_delta);

	virtual void addToRender(gfx::SceneRenderer&) const;
	WeaponClassId::Type weaponId() const { return m_weapon_id; }
	
	void printStatusInfo() const;


protected:
	void issueNextOrder();
	void issueMoveOrder();

	void setWeapon(WeaponClassId::Type);
	void setSequence(ActionId::Type);
	void lookAt(int3 pos);

	void animate(double current_time);
	void onAnimFinished();

	// orders
	bool m_issue_next_order;
	Order m_order;
	Order m_next_order;

	// animation state
	bool m_looped_anim;
	int m_seq_id, m_frame_id;
	int m_dir, m_target_dir;
	double m_last_time;

	// movement state
	int3 m_last_pos;
	float m_path_t;
	int m_path_pos;
	vector<int3> m_path;

	ActionId::Type m_action_id;
	StanceId::Type m_stance_id;
	WeaponClassId::Type m_weapon_id;

	gfx::PSprite m_sprite;
	AnimationMap m_anim_map;
};




#endif
