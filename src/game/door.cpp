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

	Door::Door(const char *sprite_name, const int3 &pos, const float2 &dir)
		:Entity(sprite_name, pos) {
		m_sprite->printInfo();
		m_update_anim = false;

		for(int n = 0; n < state_count; n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			ASSERT(m_seq_ids[n] != -1);
		}
		m_state = m_target_state = state_closed;
		playSequence(m_seq_ids[m_state]);
		setDir(dir);

		setBBox(computeBBox(m_state));
	}

	void Door::interact(const Entity *interactor) {
		//TODO: no atomicity, something still might fuck up, when something changes position
		// in the same frame after door
		IBox coll_box = (IBox)computeBBox(state_opening) + (int3)pos();
		if(!m_world->isColliding(coll_box, this))
			m_target_state = isOpened()? state_closed : state_opened;
	}
	
	IBox Door::computeBBox(State state) const {	
		float3 size = m_sprite->boundingBox();
		float maxs = max(size.x, size.z);
		
		FBox box;
		if(state == state_closing || state == state_opening)
			box = FBox(0, 0, 0, maxs, size.y, maxs);
		else if(state == state_opened)
			box = FBox(0, 0, 0, size.z, size.y, size.x);
		else if(state == state_closed)
			box = FBox(0, 0, 0, size.x, size.y, size.z);
		
		return rotateY(FBox(box), size * 0.5f, dirAngle());
	}

	void Door::think() {
		const float2 dir = actualDir();
		
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
			setBBox(computeBBox(m_state));
			playSequence(m_seq_ids[m_state]);
			m_update_anim = false;
		}
	}

	void Door::onAnimFinished() {
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
