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

	DEFINE_ENUM(DoorClassId,
		rotating,
		sliding,
		rotating_in,
		rotating_out
	)

	DEFINE_ENUM(DoorSoundType,
		open,
		close,
	)

	DEFINE_ENUM(DoorState,
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
		DoorClassId class_id;
		EnumMap<DoorSoundType, SoundId> sound_ids;
		EnumMap<DoorState, int> seq_ids;
	};

	class Door: public EntityImpl<Door, DoorProto, EntityId::door>
	{
	public:
		Door(Stream&);
		Door(const XMLNode&);
		Door(const DoorProto &proto);

		FlagsType flags() const override { return Flags::door | Flags::dynamic_entity | Flags::occluding | Flags::colliding; }
		
		void interact(const Entity*) override;
		void onSoundEvent() override;

		bool isOpened() const { return m_state == DoorState::opened_in || m_state == DoorState::opened_out; }
		DoorClassId classId() const { return m_proto.class_id; }
		void setKey(const Item&);
		void setDirAngle(float angle) override;

		XMLNode save(XMLNode&) const override;
		void save(Stream&) const override;
		const FBox boundingBox() const override;

		void onImpact(DamageType damage_type, float damage, const float3 &force, EntityRef source) override;
		
	private:
		void initialize();
		void initializeOpenDir();

		void think() override;
		void onAnimFinished() override;
		FBox computeBBox(DoorState) const;
		void changeState(DoorState);

		FBox m_bbox;
		Item m_key;
		DoorState m_state;
		double m_close_time;
		float2 m_open_in_dir;
		bool m_update_anim;
	};

};

#endif
