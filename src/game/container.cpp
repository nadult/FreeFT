/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/container.h"
#include "game/actor.h"
#include "game/sprite.h"
#include "game/world.h"
#include "sys/xml.h"
#include <cstdio>

namespace game {

	DEFINE_ENUM(ContainerSoundType,
		"open",
		"close"
	)

	static const char *s_seq_names[ContainerState::count] = {
		"Closed",
		"Opened",
		"Opening",
		"Closing",
	};


	ContainerProto::ContainerProto(const TupleParser &parser) :ProtoImpl(parser) {
		sprite_name = parser("sprite_name");
		ASSERT(!sprite_name.empty());
		name = parser("name");

		const char *sound_prefix = parser("sound_prefix");
		for(int n = 0; n < ContainerSoundType::count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, ContainerSoundType::toString(n));
			sound_ids[n] = SoundId(name);
		}

		is_always_opened = false;
		for(int n = 0; n < ContainerState::count; n++) {
			seq_ids[n] = sprite->findSequence(s_seq_names[n]);
			if(seq_ids[n] == -1)
				is_always_opened = true;
		}
	}

	Container::Container(Stream &sr) :EntityImpl(sr) {
		sr.unpack(m_state, m_target_state);
		m_update_anim = false;
		sr >> m_inventory;
	}

	Container::Container(const XMLNode &node) :EntityImpl(node) {
		initialize();
	}
	Container::Container(const ContainerProto &proto, const float3 &pos) :EntityImpl(proto) {
		initialize();
		setPos(pos);
	}

	void Container::save(Stream &sr) const {
		EntityImpl::save(sr);
		sr.pack(m_state, m_target_state);
		sr << m_inventory;
	}

	XMLNode Container::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		return node;
	}

	void Container::initialize() {
		m_update_anim = false;
		m_state = m_target_state = m_proto.is_always_opened? ContainerState::opened : ContainerState::closed;
		if(!m_proto.is_always_opened)
			playSequence(m_proto.seq_ids[m_state], false);
	}
	
	void Container::setKey(const Item &key) {
		DASSERT(key.type() == ItemType::other);
		m_key = key;
	}

	void Container::interact(const Entity *interactor) {
		if(m_proto.is_always_opened)
			return;
		
		if(isOpened())
			close();
		else {
			if(!m_key.isDummy()) {
				const Actor *actor = dynamic_cast<const Actor*>(interactor);
				if(!actor || actor->inventory().find(m_key) == -1) {
					printf("Key required!\n");
					return;
				}
			}
			open();
		}
	}
	
	void Container::onSoundEvent() {
		bool is_opening = m_state == ContainerState::opening;
		ContainerSoundType::Type sound_type = is_opening? ContainerSoundType::opening : ContainerSoundType::closing;
		world()->playSound(m_proto.sound_ids[sound_type], pos());
	}
	
	void Container::open() {
		m_target_state = ContainerState::opened;
	}

	void Container::close() {
		m_target_state = ContainerState::closed;
	}

	void Container::think() {
		if(m_proto.is_always_opened)
			return;

		if(m_target_state != m_state) {
			if(m_state == ContainerState::closed && m_target_state == ContainerState::opened) {
				m_state = ContainerState::opening;
				m_update_anim = true;
			}
			if(m_state == ContainerState::opened && m_target_state == ContainerState::closed) {
				m_state = ContainerState::closing;
				m_update_anim = true;
			}
		}
		if(m_update_anim) {
			world()->replicate(this);

			playSequence(m_proto.seq_ids[m_state]);
			m_update_anim = false;
		}
	}

	void Container::onAnimFinished() {
		if(m_proto.is_always_opened)
			return;

		if(m_state == ContainerState::opening) {
			m_state = ContainerState::opened;
			m_update_anim = true;
		}
		else if(m_state == ContainerState::closing) {
			m_state = ContainerState::closed;
			m_update_anim = true;
		}
	}

}
