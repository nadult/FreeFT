#include "game/container.h"
#include "gfx/sprite.h"

namespace game {

	static const char *s_seq_names[Container::state_count] = {
		"Closed",
		"Opened",
		"Opening",
		"Closing",
	};

	Container::Container(const char *sprite_name, const float3 &pos)
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

	void Container::interact(const Entity *interactor) {
		if(m_is_always_opened)
			return;
		
		if(isOpened())
			close();
		else
			open();
	}

	void Container::open() {
		m_target_state = state_opened;
	}

	void Container::close() {
		m_target_state = state_closed;
	}

	void Container::think() {
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

	void Container::onAnimFinished() {
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
