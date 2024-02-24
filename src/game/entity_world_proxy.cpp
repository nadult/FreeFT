// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/base.h"
#include "game/game_mode.h"
#include "game/world.h"

namespace game {

void EntityRef::save(MemoryStream &sr) const {
	if(m_index == -1)
		encodeInt(sr, -1);
	else {
		encodeInt(sr, m_index);
		encodeInt(sr, m_unique_id);
	}
}

void EntityRef::load(MemoryStream &sr) {
	m_index = decodeInt(sr);
	m_unique_id = m_index == -1 ? -1 : decodeInt(sr);
}

static int getUniqueId() {
	static int s_counter = 0;
	return s_counter++;
}

EntityWorldProxy::EntityWorldProxy() : m_index(-1), m_unique_id(getUniqueId()), m_world(nullptr) {}

EntityWorldProxy::EntityWorldProxy(MemoryStream &sr) : m_index(-1), m_world(nullptr) {
	m_unique_id = decodeInt(sr);
}

EntityWorldProxy::EntityWorldProxy(const EntityWorldProxy &rhs) : EntityWorldProxy() {}

EntityWorldProxy::~EntityWorldProxy() {}

void EntityWorldProxy::replicate() {
	if(m_world)
		m_world->replicate(m_index);
}

EntityRef EntityWorldProxy::ref() const { return EntityRef(m_index, m_unique_id); }

const FBox EntityWorldProxy::refBBox(ObjectRef ref) const {
	return m_world ? m_world->refBBox(ref) : FBox();
}

const Tile *EntityWorldProxy::refTile(ObjectRef ref) const {
	return m_world ? m_world->refTile(ref) : nullptr;
}

Entity *EntityWorldProxy::refEntity(EntityRef ref) const {
	return m_world ? m_world->refEntity(ref) : nullptr;
}

Entity *EntityWorldProxy::refEntity(ObjectRef ref) const {
	return m_world ? m_world->refEntity(ref) : nullptr;
}

void EntityWorldProxy::save(MemoryStream &sr) const { encodeInt(sr, m_unique_id); }

void EntityWorldProxy::remove() {
	DASSERT(isHooked());
	m_world->m_replace_list.emplace_back(Dynamic<Entity>(nullptr), m_index);
}

void EntityWorldProxy::replaceMyself(Dynamic<Entity> &&new_entity) {
	DASSERT(isHooked());
	m_world->m_replace_list.emplace_back(std::move(new_entity), m_index);
}

void EntityWorldProxy::addEntity(Dynamic<Entity> &&new_entity) {
	DASSERT(isHooked());
	if(!m_world->isClient())
		m_world->m_replace_list.emplace_back(std::move(new_entity), -1);
}

void EntityWorldProxy::playSound(SoundId sound_id, const float3 &pos, SoundType sound_type) {
	DASSERT(isHooked());
	m_world->playSound(sound_id, pos, sound_type);
}

void EntityWorldProxy::replicateSound(SoundId sound_id, const float3 &pos, SoundType sound_type) {
	DASSERT(isHooked());
	m_world->replicateSound(sound_id, pos, sound_type);
}

void EntityWorldProxy::onKill(EntityRef target, EntityRef killer) {
	DASSERT(isHooked());
	if(m_world->gameMode())
		m_world->gameMode()->onKill(target, killer);
}

ObjectRef EntityWorldProxy::findAny(const FBox &box, const FindFilter &filter) const {
	return m_world ? m_world->findAny(box, filter) : ObjectRef();
}

void EntityWorldProxy::findAll(vector<ObjectRef> &out, const FBox &box,
							   const FindFilter &filter) const {
	if(m_world)
		m_world->findAll(out, box, filter);
}

Intersection EntityWorldProxy::trace(const Segment3F &segment, const FindFilter &filter) const {
	return m_world ? m_world->trace(segment, filter) : Intersection();
}

double EntityWorldProxy::timeDelta() const { return m_world ? m_world->timeDelta() : 0.0f; }

double EntityWorldProxy::currentTime() const { return m_world ? m_world->currentTime() : 0.0f; }

float EntityWorldProxy::random() const { return m_world ? m_world->random() : 0.0f; }

bool EntityWorldProxy::isClient() const { return m_world && m_world->isClient(); }

bool EntityWorldProxy::isServer() const { return m_world && m_world->isServer(); }

void EntityWorldProxy::hook(World *world, int index) {
	DASSERT(!isHooked());
	m_world = world;
	m_index = index;
}

void EntityWorldProxy::unhook() {
	DASSERT(isHooked());
	m_world = nullptr;
	m_index = -1;
}

bool EntityWorldProxy::isHooked() const { return m_world != nullptr && m_index != -1; }

}
