#ifndef GAME_CONTAINER_H
#define GAME_CONTAINER_H

#include "game/entity.h"


namespace game
{

	class Container: public Entity
	{
	public:
		Container(const char *sprite_name, const int3 &pos);
		virtual ColliderType colliderType() const { return collider_static; }

		void open();
		void close();

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened; }
		bool isAlwaysOpened() const { return m_is_always_opened; }

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

		State m_state, m_target_state;
		bool m_is_always_opened;
		bool m_update_anim;

		int m_seq_ids[state_count];
	};
};

#endif
