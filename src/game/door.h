/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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

	public:
		Door(Stream&);
		Door(const XMLNode&);
		Door(const char *sprite_name, Type type, const float3 &pos);

		virtual ColliderFlags colliderType() const { return collider_dynamic_nv; }
		virtual EntityId::Type entityType() const { return EntityId::door; }
		virtual Entity *clone() const;
		
		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened_in || m_state == state_opened_out; }
		Type type() const { return m_type_id; }
		void setKey(const Item&);
		virtual void setDirAngle(float angle);

		virtual void save(XMLNode&) const;
		virtual void save(Stream&) const;
		
	private:
		void initialize(Type type);

		virtual void think();
		virtual void onAnimFinished();
		FBox computeBBox(State) const;

		State m_state;
		Type m_type_id;
		bool m_update_anim;
		Item m_key;
		double m_close_time;

		int m_seq_ids[state_count];
	};
};

#endif
