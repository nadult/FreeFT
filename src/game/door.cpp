/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/door.h"
#include "game/world.h"
#include "game/actor.h"
#include "game/sprite.h"
#include "sys/xml.h"
#include <cstdio>

namespace game {

	DEFINE_ENUM(DoorClassId,
		"rotating",
		"sliding",
		"rotating_in",
		"rotating_out"
	)

	DEFINE_ENUM(DoorSoundType,
		"open",
		"close"
	)

	static const char *s_seq_names[DoorState::count] = {
		"Closed",
		"OpenedIn",
		"OpeningIn",
		"ClosingIn",
		"OpenedOut",
		"OpeningOut",
		"ClosingOut",
	};
	
	DoorProto::DoorProto(const TupleParser &parser) :ProtoImpl(parser) {
		ASSERT(!is_dummy);
		name = parser("name");

		class_id = DoorClassId::fromString(parser("class_id"));
		const char *sound_prefix = parser("sound_prefix");

		for(int n = 0; n < DoorSoundType::count; n++) {
			char name[256];
			snprintf(name, sizeof(name), "%s%s", sound_prefix, DoorSoundType::toString(n));
			sound_ids[n] = SoundId(name);
		}
		
		for(int n = 0; n < DoorState::count; n++)
			seq_ids[n] = sprite->findSequence(s_seq_names[n]);
		
		if(seq_ids[DoorState::closed] == -1)
			THROW("Missing sequence: %s", s_seq_names[DoorState::closed]);

		bool can_open_in =
			seq_ids[DoorState::opened_in] != -1 &&
			seq_ids[DoorState::opening_in] != -1 &&
			seq_ids[DoorState::closing_in] != -1;
		bool can_open_out =
			seq_ids[DoorState::opened_out] != -1 &&
			seq_ids[DoorState::opening_out] != -1 &&
			seq_ids[DoorState::closing_out] != -1;

		bool error = false;

		if((class_id == DoorClassId::sliding || class_id == DoorClassId::rotating ||
				class_id == DoorClassId::rotating_in) && !can_open_in)
			error = true;
		if((class_id == DoorClassId::rotating || class_id == DoorClassId::rotating_out) && !can_open_out)
			error = true;
		if(class_id == DoorClassId::sliding && can_open_out)
			error = true;
		
		if(error)		
			THROW("Invalid sequence combination");
	}

	struct Transition { DoorState::Type current, target, result; };
	static Transition s_transitions[] = {
		{ DoorState::closed,		DoorState::opened_in,	DoorState::opening_in },
		{ DoorState::closed,		DoorState::opened_out,	DoorState::opening_out },
		{ DoorState::opened_in,		DoorState::closed,		DoorState::closing_in },
		{ DoorState::opened_out,	DoorState::closed,		DoorState::closing_out },
	};

	Door::Door(Stream &sr) :EntityImpl(sr) {
		sr.unpack(m_close_time, m_update_anim, m_state);
		m_bbox = computeBBox(m_state);
		initializeOpenDir();
	}

	Door::Door(const XMLNode &node) :EntityImpl(node) {
		initialize();
	}
		
	Door::Door(const DoorProto &proto) :EntityImpl(proto) {
		initialize();
	}
	
	XMLNode Door::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		return node;
	}

	void Door::save(Stream &sr) const {
		EntityImpl::save(sr);
		sr.pack(m_close_time, m_update_anim, m_state);
	}

	void Door::initialize() {
		m_close_time = -1.0;
		m_update_anim = false;
		
		m_state = DoorState::closed;
		playSequence(m_proto.seq_ids[m_state], false);
		m_bbox = computeBBox(m_state);
		initializeOpenDir();
	}
		
	void Door::initializeOpenDir() {
		float3 center = computeBBox(DoorState::closed).center();
		
		if(classId() == DoorClassId::rotating_in)
			m_open_in_dir = (computeBBox(DoorState::opening_in).center() - center).xz();
		else
			m_open_in_dir = -(computeBBox(DoorState::opening_out).center() - center).xz();
		m_open_in_dir = m_open_in_dir / length(m_open_in_dir);
	}
		
	void Door::setDirAngle(float angle) {
		Entity::setDirAngle(angle);
		m_bbox = computeBBox(m_state);
	}
	
	void Door::setKey(const Item &key) {
		DASSERT(key.type() == ItemType::other);
		m_key = key;
	}

	void Door::interact(const Entity *interactor) {
		DoorState::Type target;

		if(isOpened())
			target = DoorState::closed;
		else {
			if(!m_key.isDummy()) {
				const Actor *actor = dynamic_cast<const Actor*>(interactor);
				if(!actor || actor->inventory().find(m_key) == -1) {
					printf("Key required!\n");
					return;
				}
			}

			target = classId() == DoorClassId::rotating_out? DoorState::opened_out : DoorState::opened_in;
			if(classId() == DoorClassId::rotating && interactor) {
				float3 interactor_pos = interactor->boundingBox().center();
				float3 dir = boundingBox().center() - interactor_pos;
				if(dot(dir.xz(), m_open_in_dir) < 0.0f)
					target = DoorState::opened_out;
			}
		}
		
		changeState(target);
		//TODO: open direction should depend on interactor's position		
	}
		
	void Door::changeState(DoorState::Type target) {
		DoorState::Type result = m_state;

		for(int n = 0; n < COUNTOF(s_transitions); n++)
			if(s_transitions[n].current == m_state && s_transitions[n].target == target) {
				result = s_transitions[n].result;
				break;
			}
		if(result == m_state)
			return;

		FBox bbox = computeBBox(result);
		bbox.min += float3(1.1f, 0.1f, 1.1f);
		bbox.max -= float3(1.1f, 0.1f, 1.1f);

		bool is_colliding = (bool)world()->findAny(bbox + pos(), {Flags::entity | Flags::colliding, ref()});

		if(is_colliding && classId() == DoorClassId::rotating && m_state == DoorState::closed && target == DoorState::opened_in) {
			target = DoorState::opened_out;
			result = DoorState::opening_out;
			bbox = computeBBox(result);
			is_colliding = (bool)world()->findAny(bbox + pos(), {Flags::entity | Flags::colliding, ref()});
		}
		if(!is_colliding) {
			m_bbox = bbox;
			m_state = result;
			m_update_anim = true;
		}
	}
		
	void Door::onSoundEvent() {
		bool is_opening = m_state == DoorState::opening_in || m_state == DoorState::opening_out;
		world()->playSound(m_proto.sound_ids[is_opening? DoorSoundType::opening : DoorSoundType::closing], pos());
	}
	
	FBox Door::computeBBox(DoorState::Type state) const {	
		float3 size = m_sprite.bboxSize();
		float maxs = max(size.x, size.z);
		
		FBox box;
		if(classId() == DoorClassId::sliding || state == DoorState::closed)
			box = FBox(float3(0, 0, 0), state == DoorState::opened_in? float3(0, 0, 0) : size);
		else if(state == DoorState::closing_in || state == DoorState::opening_in)
			box = FBox(-maxs + 1, 0, 0, 1, size.y, maxs);
		else if(state == DoorState::closing_out || state == DoorState::opening_out)
			box = FBox(0, 0, 0, maxs, size.y, maxs);
		else if(state == DoorState::opened_out)
			box = FBox(0, 0, 0, size.z, size.y, size.x);
		else if(state == DoorState::opened_in)
			box = FBox(-size.z + 1, 0, 0, 1, size.y, size.x);

		//TODO: this is still wrong
		FBox out = rotateY(box, size * 0.5f, dirAngle());
		out.min = (int3)out.min;
		out.max = (int3)out.max;
		DASSERT(classId() == DoorClassId::sliding || !out.isEmpty());
		return out;
	}
		
	const FBox Door::boundingBox() const { return m_bbox + pos(); }

	void Door::think() {
		const float2 dir = actualDir();
		World *world = this->world();
		
		if(m_update_anim) {
			world->replicate(this);
			playSequence(m_proto.seq_ids[m_state]);
			m_update_anim = false;
		}
		if(classId() == DoorClassId::sliding && m_state == DoorState::opened_in && world->currentTime() > m_close_time) {
			FBox bbox = computeBBox(DoorState::closed);
			if((bool)world->findAny(bbox + pos(), {Flags::entity | Flags::colliding, ref()})) {
				m_close_time = world->currentTime() + 1.5;
			}
			else {
				m_bbox = bbox;
				m_state = DoorState::closing_in;
				m_update_anim = true;
			}
		}
	}

	void Door::onAnimFinished() {
		for(int n = 0; n < COUNTOF(s_transitions); n++)
			if(m_state == s_transitions[n].result) {
				m_state = s_transitions[n].target;
				m_bbox = computeBBox(m_state);
				m_update_anim = true;
				if(m_state == DoorState::opened_in && classId() == DoorClassId::sliding)
					m_close_time = world()->currentTime() + 3.0;
				break;
			}
	}
		
	void Door::onImpact(DamageType::Type damage_type, float damage, const float3 &force) {
		if(!isOpened()) {
			float door_force = dot(force.xz(), m_open_in_dir);
			if(fabs(door_force) >= 1.5f && classId() != DoorClassId::sliding) {
				bool opening_in = door_force > 0.0f;

				if(classId() == DoorClassId::rotating_in && !opening_in)
					return;
				if(classId() == DoorClassId::rotating_out && opening_in)
					return;

				changeState(opening_in? DoorState::opened_in : DoorState::opened_out);
			}
		}
	}

}
