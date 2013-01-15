/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef GAME_CONTAINER_H
#define GAME_CONTAINER_H

#include "game/entity.h"
#include "game/inventory.h"

namespace game
{

	class Container: public Entity
	{
	public:
		Container(const char *sprite_name, const float3 &pos);
		virtual ColliderFlags colliderType() const { return collider_static; }
		virtual EntityId::Type entityType() const { return EntityId::container; }

		void open();
		void close();
		void setKey(const Item&);

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened; }
		bool isAlwaysOpened() const { return m_is_always_opened; }

		const Inventory &inventory() const { return m_inventory; }
		Inventory &inventory() { return m_inventory; }

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
		Item m_key;

		int m_seq_ids[state_count];
		Inventory m_inventory;
	};
};

#endif
