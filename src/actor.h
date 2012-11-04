#ifndef ACTOR_H
#define ACTOR_H

#include "entity.h"
#include "gfx/sprite.h"

class Actor: public Entity {
public:
	Actor(const char *spr_name, int3 pos);

	enum OrderId {
		oDoNothing,
		oChangeStance,
		oMove,

		orderCount,
	};

	enum StanceId {
		sStanding,
		sCrouching,
		sProne,

		stanceCount,
	};

	enum ActionId {
		aStanding,
		aWalking,
		aRunning,

		aStanceUp,
		aStanceDown,

		actionCount,
	};

	struct Order {
		OrderId m_id;
		int3 m_pos;
		int m_flags;
	};

	static Order makeMoveOrder(int3 target_pos, bool run)	{ return Order{oMove, target_pos, run?1 : 0}; }
	static Order makeDoNothingOrder() { return Order{oDoNothing, int3(), 0}; }
	static Order makeChangeStanceOrder(int target_stance) { return Order{oChangeStance, int3(), target_stance}; }

	void setNextOrder(Order order);
	void think(double current_time, double time_delta);

	virtual void addToRender(gfx::SceneRenderer&) const;

protected:
	void issueNextOrder();
	void issueMoveOrder();

	void setSequence(ActionId action);
	void lookAt(int3 pos);

	void animate(double current_time);
	void onAnimFinished();

	// orders
	bool m_issue_next_order;
	Order m_order;
	Order m_next_order;

	// animation state
	bool m_looped_anim;
	const char *m_seq_name; // TODO: store some id instead of char*
	int m_seq_id, m_frame_id;
	int m_dir;
	double m_last_time;

	// movement state
	int3 m_last_pos;
	float m_path_t;
	int m_path_pos;
	vector<int3> m_path;	

	StanceId m_stance;

	gfx::PSprite m_sprite;
};




#endif
