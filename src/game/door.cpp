#include "game/door.h"
#include "gfx/sprite.h"
#include "game/world.h"

namespace game {

	static const char *s_seq_names[Door::state_count] = {
		"Closed",
		"OpenedOut",
		"OpeningOut",
		"ClosingOut",
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
		
		//TODO: no atomicity, something still might fuck up, when something changes position
		// in the same frame after door
		IBox coll_box((int3)pos(), (int3)pos() + getBBoxSize(state_opening));
		if(!m_world->isColliding(coll_box, this)) {
			printf("nothing is blocking %d %d %d\n", coll_box.width(), coll_box.height(), coll_box.depth());
			m_target_state = isOpened()? state_closed : state_opened;
		}
	}
	
	int3 Door::getBBoxSize(State state) const {	
		int3 size = m_sprite->boundingBox();
		
		if(state == state_closing || state == state_opening)
			size.z = size.x = max(size.x, size.z);
		else if(state == state_opened)
			swap(size.x, size.z);
		return size;
	}

	void Door::think() {
		const float2 dir = actualDir();
		if(abs(dir.y) > 0.0f) { //TODO: yea...
			int3 spr_size = m_sprite->boundingBox();
			setBBoxSize(int3(spr_size.z, spr_size.y, spr_size.x));
		}
		
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
			setBBoxSize(getBBoxSize(m_state));
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
