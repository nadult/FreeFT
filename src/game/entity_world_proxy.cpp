/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/base.h"
#include "game/world.h"

namespace game {

	void EntityRef::save(Stream &sr) const {
		if(m_index == -1)
			sr.encodeInt(-1);
		else {
			sr.encodeInt(m_index);
			sr.encodeInt(m_unique_id);
		}
	}

	void EntityRef::load(Stream &sr) {
		m_index = sr.decodeInt();
		m_unique_id = m_index == -1? -1 : sr.decodeInt();
	}

	static int getUniqueId() {
		static int s_counter = 0;
		return s_counter++;
	}

	EntityWorldProxy::EntityWorldProxy() :m_index(-1), m_unique_id(getUniqueId()), m_world(nullptr) { }

	EntityWorldProxy::EntityWorldProxy(Stream &sr) :m_index(-1), m_world(nullptr) {
		m_unique_id = sr.decodeInt();
	}

	EntityWorldProxy::EntityWorldProxy(const EntityWorldProxy &rhs) :EntityWorldProxy() { }

	EntityWorldProxy::~EntityWorldProxy() { }

	void EntityWorldProxy::replicate() {
		if(m_world)
			m_world->replicate(m_index);
	}
		
	EntityRef EntityWorldProxy::ref() {
		DASSERT(isHooked());
		return EntityRef(m_index, m_unique_id);
	}
		
	const FBox EntityWorldProxy::refBBox(ObjectRef ref) const {
		return m_world? m_world->refBBox(ref) : FBox::empty();
	}

	const Tile *EntityWorldProxy::refTile(ObjectRef ref) const {
		return m_world? m_world->refTile(ref) : nullptr;
	}

	Entity *EntityWorldProxy::refEntity(EntityRef ref) const {
		return m_world? m_world->refEntity(ref) : nullptr;
	}
	
	Entity *EntityWorldProxy::refEntity(ObjectRef ref) const {
		return m_world? m_world->refEntity(ref) : nullptr;
	}

	void EntityWorldProxy::save(Stream &sr) const {
		sr.encodeInt(m_unique_id);
	}

	void EntityWorldProxy::remove() {
		DASSERT(isHooked());
		m_world->m_replace_list.emplace_back(unique_ptr<Entity>(nullptr), m_index);
	}

	void EntityWorldProxy::replaceMyself(unique_ptr<Entity> &&new_entity) {
		DASSERT(isHooked());
		m_world->m_replace_list.emplace_back(std::move(new_entity), m_index);
	}

	void EntityWorldProxy::addEntity(unique_ptr<Entity> &&new_entity) {
		DASSERT(isHooked());
		m_world->m_replace_list.emplace_back(std::move(new_entity), -1);
	}
		
	ObjectRef EntityWorldProxy::findAny(const FBox &box, const FindFilter &filter) const {
		return m_world? m_world->findAny(box, filter) : ObjectRef();
	}

	void EntityWorldProxy::findAll(vector<ObjectRef> &out, const FBox &box, const FindFilter &filter) const {
		if(m_world)
			m_world->findAll(out, box, filter);
	}

	Intersection EntityWorldProxy::trace(const Segment &segment, const FindFilter &filter) const {
		return m_world? m_world->trace(segment, filter) : Intersection();
	}

	double EntityWorldProxy::timeDelta() const {
		return m_world? m_world->timeDelta() : 0.0f;
	}

	double EntityWorldProxy::currentTime() const {
		return m_world? m_world->currentTime() : 0.0f;
	}

	bool EntityWorldProxy::isClient() const {
		return m_world && m_world->isClient();
	}

	bool EntityWorldProxy::isServer() const {
		return m_world && m_world->isServer();
	}

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

	bool EntityWorldProxy::isHooked() const {
		return m_world != nullptr && m_index != -1;
	}

}
