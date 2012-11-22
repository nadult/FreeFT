#ifndef ACTOR_H
#define ACTOR_H

#include "entity.h"
#include "gfx/sprite.h"

namespace OrderId {
	enum Type {
		do_nothing,
		change_stance,
		move,

		count,
	};
}

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

	struct Order {
		OrderId::Type m_id;
		int3 m_pos;
		int m_flags;
	};

	static Order makeMoveOrder(int3 target_pos, bool run)	{ return Order{OrderId::move, target_pos, run?1 : 0}; }
	static Order makeDoNothingOrder() { return Order{OrderId::do_nothing, int3(), 0}; }
	static Order makeChangeStanceOrder(int target_stance) { return Order{OrderId::change_stance, int3(), target_stance}; }

	void setNextOrder(Order order);
	void think(double current_time, double time_delta);

	virtual void addToRender(gfx::SceneRenderer&) const;
	WeaponClassId::Type weaponId() const { return m_weapon_id; }
	void setWeapon(WeaponClassId::Type);

protected:
	void issueNextOrder();
	void issueMoveOrder();

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
	int m_dir;
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
