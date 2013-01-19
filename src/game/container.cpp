/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "game/container.h"
#include "game/actor.h"
#include "game/sprite.h"
#include <cstdio>

namespace game {

	static const char *s_seq_names[Container::state_count] = {
		"Closed",
		"Opened",
		"Opening",
		"Closing",
	};

	Container::Container(const char *sprite_name, const float3 &pos)
		:Entity(sprite_name, pos) {
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

	void Container::setKey(const Item &key) {
		DASSERT(!key.isValid() || key.typeId() == ItemTypeId::other);
		m_key = key;
	}

	void Container::interact(const Entity *interactor) {
		if(m_is_always_opened)
			return;
		
		if(isOpened())
			close();
		else {
			if(m_key.isValid()) {
				const Actor *actor = dynamic_cast<const Actor*>(interactor);
				if(!actor || actor->inventory().find(m_key) == -1) {
					printf("Key required!\n");
					return;
				}
			}
			open();
		}
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
