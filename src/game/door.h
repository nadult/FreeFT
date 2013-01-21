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

#ifndef GAME_DOOR_H
#define GAME_DOOR_H

#include "game/entity.h"
#include "game/item.h"


namespace game
{

	class Door: public Entity
	{
	public:
		typedef DoorTypeId::Type Type;

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

		static bool testSpriteType(PSprite, Type);

		Door(const char *sprite_name, const float3 &pos, Type type, const float2 &dir = float2(1, 0));
		virtual ColliderFlags colliderType() const { return collider_dynamic_nv; }
		virtual EntityId::Type entityType() const { return EntityId::door; }
		virtual Entity *clone() const;

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened_in || m_state == state_opened_out; }
		Type type() const { return m_type; }
		void setKey(const Item&);
		
	private:
		virtual void think();
		virtual void onAnimFinished();
		FBox computeBBox(State) const;

		State m_state;
		const Type m_type;
		bool m_update_anim;
		Item m_key;

		int m_seq_ids[state_count];
	};
};

#endif
