#ifndef GAME_DOOR_H
#define GAME_DOOR_H

#include "game/entity.h"


namespace game
{

	class Door: public Entity
	{
	public:
		Door(const char *sprite_name, const int3 &pos);
		virtual ColliderType colliderType() const { return collider_dynamic; }

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened; }
		bool isAlwaysOpened() const { return m_is_always_opened; }

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
		int3 getBBoxSize(State) const;

		State m_state, m_target_state;
		bool m_is_always_opened;
		bool m_update_anim;

		int m_seq_ids[state_count];
	};
};

#endif
