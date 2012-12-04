#include "game/door.h"
#include "gfx/sprite.h"

namespace game {

	static const char *s_seq_names[Door::state_count] = {
		"Closed",
		"OpenedIn",
		"OpeningIn",
		"ClosingIn",
	};

	Door::Door(const char *sprite_name, const int3 &pos)
		:Entity(sprite_name, pos) {
		m_sprite->printInfo();
		m_update_anim = false;

		m_is_always_opened = false;
		for(int n = 0; n < state_count; n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			if(m_seq_ids[n] == -1)
				m_is_always_opened = true;
		}
		m_state = m_target_state = m_is_always_opened? state_opened : state_closed;
		if(!m_is_always_opened)
			playSequence(m_seq_ids[m_state]);
	}

	void Door::interact(const Entity *interactor) {
		if(m_is_always_opened)
			return;
		
		if(isOpened())
			close();
		else
			open();
	}

	void Door::open() {
		m_target_state = state_opened;
	}

	void Door::close() {
		m_target_state = state_closed;
	}

	void Door::think() {
		if(m_is_always_opened)
			return;

		if(m_target_state != m_state) {
			if(m_state == state_closed && m_target_state == state_opened) {
				m_state = state_opening;
				m_update_anim = true;
			}
			if(m_state == state_opened && m_target_state == state_closed) {
				m_state = state_closing;
				m_update_anim = true;
			}
		}
		if(m_update_anim) {
			playSequence(m_seq_ids[m_state]);
			m_update_anim = false;
		}
	}

	void Door::onAnimFinished() {
		if(m_is_always_opened)
			return;

		if(m_state == state_opening) {
			m_state = state_opened;
			m_update_anim = true;
		}
		else if(m_state == state_closing) {
			m_state = state_closed;
			m_update_anim = true;
		}
	}

}
