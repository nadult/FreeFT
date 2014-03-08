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

	DECLARE_ENUM(DoorState,
		closed,

		opened_in,
		opening_in,
		closing_in,

		opened_out,
		opening_out,
		closing_out
	);

	struct DoorProto: public ProtoImpl<DoorProto, EntityProto, ProtoId::door> {
		DoorProto(const TupleParser&);

		string sprite_name;
		string name;
		DoorClassId::Type class_id;
		SoundId sound_ids[DoorSoundType::count];
		int seq_ids[DoorState::count];
	};

	class Door: public EntityImpl<Door, DoorProto, EntityId::door>
	{
	public:
		Door(Stream&);
		Door(const XMLNode&);
		Door(const DoorProto &proto);

		virtual ColliderFlags colliderType() const { return collider_dynamic_nv; }
		virtual EntityId::Type entityType() const { return EntityId::door; }
		virtual Entity *clone() const;
		
		virtual void interact(const Entity*);
		virtual void onSoundEvent();

		bool isOpened() const { return m_state == DoorState::opened_in || m_state == DoorState::opened_out; }
		DoorClassId::Type classId() const { return m_proto.class_id; }
		void setKey(const Item&);
		virtual void setDirAngle(float angle);

		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;
		virtual const FBox boundingBox() const;
		
	private:
		void initialize();

		virtual void think();
		virtual void onAnimFinished();
		FBox computeBBox(DoorState::Type) const;

		FBox m_bbox;
		Item m_key;
		DoorState::Type m_state;
		double m_close_time;
		bool m_update_anim;
	};
};

#endif
