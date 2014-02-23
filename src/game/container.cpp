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

	ContainerDesc::ContainerDesc(const TupleParser &parser) :Tuple(parser) {
		sprite_name = parser("sprite_name");
		ASSERT(!sprite_name.empty());
		name = parser("name");

		const char *sound_prefix = parser("sound_prefix");
		for(int n = 0; n < ContainerSoundType::count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, ContainerSoundType::toString(n));
			sound_ids[n] = SoundId(name);
		}
	}

	static const char *s_seq_names[Container::state_count] = {
		"Closed",
		"Opened",
		"Opening",
		"Closing",
	};

	Container::Container(Stream &sr) {
		int desc_id = sr.decodeInt();
		ASSERT(desc_id >= 0 && desc_id < ContainerDesc::count());
		m_desc = &ContainerDesc::get(desc_id);

		Entity::initialize(m_desc->sprite_name.c_str());
		loadEntityParams(sr);
		sr.unpack(m_state, m_target_state);
		
		m_update_anim = false;
		m_is_always_opened = false;
		for(int n = 0; n < state_count; n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			if(m_seq_ids[n] == -1)
				m_is_always_opened = true;
		}
	}

	Container::Container(const XMLNode &node) :Entity(node) {
		m_desc = &ContainerDesc::get(node.attrib("container_id"));
		initialize();
	}
	Container::Container(const ContainerDesc &desc, const float3 &pos) :Entity(desc.sprite_name), m_desc(&desc) {
		initialize();
		setPos(pos);
	}

	void Container::save(Stream &sr) const {
		sr.encodeInt(m_desc->idx);
		saveEntityParams(sr);
		sr.pack(m_state, m_target_state);
	}

	XMLNode Container::save(XMLNode &parent) const {
		XMLNode node = Entity::save(parent);
		node.addAttrib("container_id", node.own(m_desc->id));
		return node;
	}

	void Container::initialize() {
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
	
	Entity *Container::clone() const {
		return new Container(*this);
	}

	void Container::setKey(const Item &key) {
		DASSERT(key.type() == ItemType::other);
		m_key = key;
	}

	void Container::interact(const Entity *interactor) {
		if(m_is_always_opened)
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
		bool is_opening = m_state == state_opening;
		ContainerSoundType::Type sound_type = is_opening? ContainerSoundType::opening : ContainerSoundType::closing;
		world()->playSound(m_desc->sound_ids[sound_type], pos());
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
			world()->replicate(this);

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
