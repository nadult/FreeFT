/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_DOOR_H
#define GAME_DOOR_H

#include "game/entity.h"
#include "game/item.h"
#include "sys/data_sheet.h"

namespace game
{

	DECLARE_ENUM(DoorClassId,
		rotating,
		sliding,
		rotating_in,
		rotating_out
	)

	DECLARE_ENUM(DoorSoundType,
		opening,
		closing
	)

	struct DoorDesc: public Tuple, TupleImpl<DoorDesc> {
		DoorDesc(const TupleParser&);

		string sprite_name;
		string name;
		DoorClassId::Type class_id;
		SoundId sound_ids[DoorSoundType::count];
	};

	class Door: public Entity
	{
	public:
		typedef DoorClassId::Type Class;

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

		static bool testSpriteType(PSprite, Class);

	public:
		Door(Stream&);
		Door(const XMLNode&);
		Door(const DoorDesc &desc, const float3 &pos);

		virtual ColliderFlags colliderType() const { return collider_dynamic_nv; }
		virtual EntityId::Type entityType() const { return EntityId::door; }
		virtual Entity *clone() const;
		
		virtual void interact(const Entity*);
		virtual void onSoundEvent();

		bool isOpened() const { return m_state == state_opened_in || m_state == state_opened_out; }
		Class classId() const { return m_desc->class_id; }
		void setKey(const Item&);
		virtual void setDirAngle(float angle);

		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;
		
	private:
		void initialize();

		virtual void think();
		virtual void onAnimFinished();
		FBox computeBBox(State) const;

		State m_state;
		Item m_key;
		double m_close_time;

		int m_seq_ids[state_count];
		bool m_update_anim;

		const DoorDesc *m_desc;
	};
};

#endif
