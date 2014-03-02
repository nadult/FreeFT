/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_CONTAINER_H
#define GAME_CONTAINER_H

#include "game/entity.h"
#include "game/inventory.h"
#include "sys/data_sheet.h"

namespace game
{
	
	DECLARE_ENUM(ContainerSoundType,
		opening,
		closing
	)

	DECLARE_ENUM(ContainerState,
		closed,
		opened,
		opening,
		closing
	);


	struct ContainerProto: public ProtoImpl<ContainerProto, EntityProto, ProtoId::container> {
		ContainerProto(const TupleParser&);

		string name;
		SoundId sound_ids[ContainerSoundType::count];
		int seq_ids[ContainerState::count];
		bool is_always_opened;
	};


	class Container: public EntityImpl<Container, ContainerProto, EntityId::container>
	{
	public:
		Container(Stream&);
		Container(const XMLNode&);
		Container(const ContainerProto&, const float3 &pos);
		//TODO: remove pos from initializer (everywhere)

		virtual ColliderFlags colliderType() const { return collider_static; }

		void open();
		void close();
		void setKey(const Item&);

		virtual void interact(const Entity*);
		virtual void onSoundEvent();

		bool isOpened() const { return m_state == ContainerState::opened; }
		bool isAlwaysOpened() const { return m_proto.is_always_opened; }

		const Inventory &inventory() const { return m_inventory; }
		Inventory &inventory() { return m_inventory; }
		
		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;
		
	private:
		virtual void think();
		virtual void onAnimFinished();
		void initialize();

		Inventory m_inventory;
		Item m_key;

		ContainerState::Type m_state, m_target_state;
		bool m_is_always_opened;
		bool m_update_anim;
	};
};

#endif
