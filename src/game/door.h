#ifndef GAME_DOOR_H
#define GAME_DOOR_H

#include "game/entity.h"


namespace game
{

	class Door: public Entity
	{
	public:
		Door(const char *sprite_name, const int3 &pos, const float2 &dir = float2(1, 0));
		virtual ColliderType colliderType() const { return collider_dynamic; }

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened; }
		
		enum Type {
			type_sliding,
			type_rotating,
		};

		enum State {
			state_closed,
			state_opened,
			state_opening,
			state_closing,

			state_count
		};

	private:
		virtual void think();
		virtual void onAnimFinished();
		IBox computeBBox(State) const;

		State m_state, m_target_state;
		bool m_update_anim;

		int m_seq_ids[state_count];
	};
};

#endif
