#ifndef GAME_DOOR_H
#define GAME_DOOR_H

#include "game/entity.h"


namespace game
{

	class Door: public Entity
	{
	public:
		enum Type {
			type_sliding,
			type_rotating,
			type_rotating_in,
			type_rotating_out,
		};

		enum State {
			state_closed,

			state_opened_in,
			state_opening_in,
			state_closing_in,

			state_opened_out,
			state_opening_out,
			state_closing_out,

			state_count
		};

		Door(const char *sprite_name, const float3 &pos, Type type, const float2 &dir = float2(1, 0));
		virtual ColliderFlags colliderType() const { return collider_dynamic_nv; }

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened_in || m_state == state_opened_out; }
		Type type() const { return m_type; }
		
	private:
		virtual void think();
		virtual void onAnimFinished();
		FBox computeBBox(State) const;

		State m_state;
		const Type m_type;
		bool m_update_anim;

		int m_seq_ids[state_count];
	};
};

#endif
