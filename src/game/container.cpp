// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/container.h"
#include "game/actor.h"
#include "game/sprite.h"
#include <cstdio>

namespace game {

static const EnumMap<ContainerState, const char *> s_seq_names = {{
	"Closed",
	"Opened",
	"Opening",
	"Closing",
}};

ContainerProto::ContainerProto(const TupleParser &parser) : ProtoImpl(parser) {
	sprite_name = parser("sprite_name");
	ASSERT(!sprite_name.empty());
	name = parser("name");

	const char *sound_prefix = parser("sound_prefix");
	for(auto cst : all<ContainerSoundType>) {
		char name[256];
		snprintf(name, sizeof(name), "%s%s", sound_prefix, toString(cst));
		sound_ids[cst] = SoundId(name);
	}

	is_always_opened = false;
	for(auto cs : all<ContainerState>) {
		seq_ids[cs] = sprite->findSequence(s_seq_names[cs]);
		if(seq_ids[cs] == -1)
			is_always_opened = true;
	}
}

Container::Container(MemoryStream &sr) : EntityImpl(sr) {
	sr.unpack(m_state, m_target_state);
	m_update_anim = false;
	m_inventory.load(sr);
}

Container::Container(CXmlNode node) : EntityImpl(node) { initialize(); }
Container::Container(const ContainerProto &proto) : EntityImpl(proto) { initialize(); }

void Container::save(MemoryStream &sr) const {
	EntityImpl::save(sr);
	sr.pack(m_state, m_target_state);
	m_inventory.save(sr);
}

XmlNode Container::save(XmlNode parent) const {
	auto node = EntityImpl::save(parent);
	return node;
}

void Container::initialize() {
	m_update_anim = false;
	m_state = m_target_state =
		m_proto.is_always_opened ? ContainerState::opened : ContainerState::closed;
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
			const Actor *actor = dynamic_cast<const Actor *>(interactor);
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
	ContainerSoundType sound_type =
		is_opening ? ContainerSoundType::opening : ContainerSoundType::closing;
	replicateSound(m_proto.sound_ids[sound_type], pos());
}

void Container::open() { m_target_state = ContainerState::opened; }

void Container::close() { m_target_state = ContainerState::closed; }

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
		replicate();
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
	} else if(m_state == ContainerState::closing) {
		m_state = ContainerState::closed;
		m_update_anim = true;
	}
}

}
