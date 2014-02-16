/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/entity.h"
#include "game/sprite.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include "sys/profiler.h"
#include "sys/xml.h"

using namespace gfx;


namespace game {

	static int s_unique_id = 0;

	void Entity::initialize(const char *sprite_name) {
		m_sprite = Sprite::mgr[sprite_name];
		m_max_screen_rect = m_sprite->getMaxRect();
		m_bbox = FBox(float3(0, 0, 0), (float3)m_sprite->boundingBox());

		m_dir_idx = 0;
		m_dir_angle = 0.0f;
		m_pos = float3(0.0f, 0.0f, 0.0f);
		m_seq_id = -1;
		playSequence(0);
	}

	enum {
		flag_compressed = 1,
		flag_is_looped = 2,
		flag_is_finished = 4,
	};

	void Entity::saveEntityParams(Stream &sr) const {
		bool can_compress =
			m_seq_id >= 0 && m_seq_id <= 255 &&
			m_frame_id >= 0 && m_frame_id <= 255 &&
			m_dir_idx >= 0 && m_dir_idx <= 255;

		u8 flags = (can_compress? flag_compressed : 0) | (m_is_looped? flag_is_looped : 0) | (m_is_finished? flag_is_finished : 0);

		sr.pack(flags, m_pos, m_dir_angle);
		sr.encodeInt(m_unique_id);

		if(can_compress)
			sr.pack(u8(m_seq_id), u8(m_frame_id), u8(m_dir_idx));
		else
			sr.pack(m_seq_id, m_frame_id, m_dir_idx);
	}

	void Entity::loadEntityParams(Stream &sr) {
		u8 flags;
		sr.unpack(flags, m_pos, m_dir_angle);
		m_unique_id = sr.decodeInt();

		m_is_finished = flags & flag_is_finished;
		m_is_looped = flags & flag_is_looped;

		if(flags & flag_compressed) {
			u8 frame_id, seq_id, dir_idx;
			sr.unpack(seq_id, frame_id, dir_idx);
			m_frame_id = frame_id;
			m_seq_id = seq_id;
			m_dir_idx = dir_idx;
		}
		else
			sr.unpack(m_seq_id, m_frame_id, m_dir_idx);
	}

	Entity::Entity() :m_to_be_removed(false), m_grid_index(-1), m_unique_id(s_unique_id++), m_first_ref(nullptr) { }

	Entity::Entity(const char *sprite_name) :Entity() {
		initialize(sprite_name);
	}

	Entity::Entity(const Entity &rhs) :Entity() {
		operator=(rhs);
	}

	Entity::Entity(Stream &sr) :Entity() {
		char sprite_name[256];
		sr.loadString(sprite_name, sizeof(sprite_name));

		initialize(sprite_name);
		loadEntityParams(sr);
	}
	
	Entity::Entity(const XMLNode &node) :Entity() {
		initialize(node.attrib("sprite"));
		m_pos = node.float3Attrib("pos");
		m_dir_angle = node.floatAttrib("angle");
		m_dir_idx = m_sprite->findDir(m_seq_id, m_dir_angle);
	}

	XMLNode Entity::save(XMLNode &parent) const {
		XMLNode node = parent.addChild(EntityId::toString(entityType()));
		node.addAttrib("pos", m_pos);
		node.addAttrib("sprite", node.own(m_sprite->resourceName()));
		node.addAttrib("angle", m_dir_angle);
		return node;
	}

	void Entity::save(Stream &sr) const {
		ASSERT(sr.isSaving());
		sr.saveString(m_sprite->resourceName());
		saveEntityParams(sr);
	}

	void Entity::operator=(const Entity &rhs) {
		m_sprite = rhs.m_sprite;
		m_max_screen_rect = rhs.m_max_screen_rect;
		m_pos = rhs.m_pos;
		m_bbox = rhs.m_bbox;
		
		m_seq_id = rhs.m_seq_id;
		m_frame_id = rhs.m_frame_id;
		m_is_looped = rhs.m_is_looped;
		m_is_finished = rhs.m_is_finished;
		m_dir_idx = rhs.m_dir_idx;
		m_dir_angle = rhs.m_dir_angle;
	}

	Entity::~Entity() {
		if(m_first_ref) {
			EntityRef* ref = m_first_ref;
			do {
				ref->m_node = nullptr;
				ref = ref->m_next;
			}
			while(ref != m_first_ref);
		}
	}

	void Entity::roundPos() {
		m_pos = (int3)(m_pos + float3(0.5f, 0, 0.5f));
	}

	void Entity::remove() {
		m_to_be_removed = true;
	}

	void Entity::setPos(const float3 &new_pos) {
		m_pos = new_pos;
	}

	const FBox Entity::boundingBox() const {
		return m_bbox + pos();
	}

	const IRect Entity::screenRect() const {
		return m_max_screen_rect + (int2)worldToScreen(pos());
	}
	
	const IRect Entity::currentScreenRect() const {
		return m_sprite->getRect(m_seq_id, m_frame_id, m_dir_idx) + (int2)worldToScreen(pos());
	}

	void Entity::addToRender(gfx::SceneRenderer &out) const {
		//PROFILE("Entity::addToRender");
		IRect rect = m_sprite->getRect(m_seq_id, m_frame_id, m_dir_idx);
		if(!areOverlapping(out.targetRect(), rect + (int2)worldToScreen(m_pos)))
			return;

		FBox bbox = m_bbox;
		if(shrinkRenderedBBox())
			bbox.min.y = min(bbox.min.y + 1.0f, bbox.max.y - 0.5f);

		FRect tex_rect;
		PTexture tex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir_idx, tex_rect);
		out.add(tex, rect, m_pos, bbox, Color::white, tex_rect);

		bbox += pos();
		if(world() && world()->isColliding(bbox, this))
			out.addBox(bbox, Color::red);
	}
		
	bool Entity::testPixel(const int2 &screen_pos) const {
		return m_sprite->testPixel(screen_pos - (int2)worldToScreen(pos()), m_seq_id, m_frame_id, m_dir_idx);
	}

	void Entity::handleEventFrame(const Sprite::Frame &frame) {
		DASSERT(frame.id <= Sprite::ev_first_specific);

		if(frame.id == Sprite::ev_fire)
			onFireEvent(int3(frame.params[0], frame.params[1], frame.params[2]));
		else if(frame.id == Sprite::ev_hit)
			onHitEvent();
		else if(frame.id == Sprite::ev_sound)
			onSoundEvent();
		else if(frame.id == Sprite::ev_step_left || frame.id == Sprite::ev_step_right)
			onStepEvent(frame.id == Sprite::ev_step_left);
		else if(frame.id == Sprite::ev_pickup)
			onPickupEvent();
	}

	void Entity::changeSprite(const char *new_name, bool update_bbox) {
		string sequence_name = (*m_sprite)[m_seq_id].name;
		m_sprite = Sprite::mgr[new_name];
		int new_seq_id = m_sprite->findSequence(sequence_name.c_str());
		if(new_seq_id == -1)
			new_seq_id = 0;
		m_seq_id = -1;
		m_is_looped = false;
		m_dir_idx = m_sprite->findDir(new_seq_id, m_dir_angle);
		playSequence(new_seq_id);

		if(update_bbox)
			m_bbox = FBox(float3(0, 0, 0), (float3)m_sprite->boundingBox());

		m_max_screen_rect = m_sprite->getMaxRect();
	}

	void Entity::playSequence(int seq_id) {
		DASSERT(seq_id >= 0 && seq_id < m_sprite->size());

		m_is_finished = false;
		if(seq_id != m_seq_id || !m_is_looped) {
			int frame_count = m_sprite->frameCount(seq_id);
			m_frame_id = 0;

			while(m_frame_id < frame_count && m_sprite->frame(seq_id, m_frame_id).id < 0) {
				if(m_sprite->frame(seq_id, m_frame_id).id <= Sprite::ev_first_specific)
					handleEventFrame(m_sprite->frame(seq_id, m_frame_id));
				m_frame_id++;
			}

			DASSERT(m_frame_id < frame_count);
		}
		if(seq_id != m_seq_id) {
			m_seq_id = seq_id;
			m_dir_idx = m_sprite->findDir(m_seq_id, m_dir_angle);
			m_is_looped = m_sprite->isSequenceLooped(seq_id);
		}
	}

	void Entity::nextFrame() {
		if(m_is_finished)
			return;

		int prev_frame = m_frame_id;
		m_frame_id++;
		int frame_count = m_sprite->frameCount(m_seq_id);

		while(m_frame_id < frame_count && m_sprite->frame(m_seq_id, m_frame_id).id < 0) {
			const Sprite::Frame &frame = m_sprite->frame(m_seq_id, m_frame_id);
			if(frame.id == Sprite::ev_repeat_all)
				m_frame_id = 0;
			else if(frame.id == Sprite::ev_jump_to_frame) 
				m_frame_id = frame.params[0];
			else {
				if(frame.id <= Sprite::ev_first_specific)
					handleEventFrame(frame);
				m_frame_id++;
			}
		}

		if(m_frame_id == frame_count) {
			m_is_finished = true;
			m_frame_id = prev_frame;
			onAnimFinished();
		}
	}

	void Entity::setDir(const float2 &vec) {
		setDirAngle(vectorToAngle(vec));
	}

	void Entity::setDirAngle(float angle) {
		m_dir_angle = angle;
		m_dir_idx = m_sprite->findDir(m_seq_id, angle);
	}

	const float2 Entity::dir() const {
		return angleToVector(dirAngle());
	}

	float Entity::actualDirAngle() const {
		int dir_count = m_sprite->dirCount(m_seq_id);
		return float(m_dir_idx) * (constant::pi * 2.0f) / float(dir_count) + constant::pi;
	}

	const float2 Entity::actualDir() const {
		return angleToVector(actualDirAngle());
	}

	bool areAdjacent(const Entity &a, const Entity &b) {
		FBox box_a = a.boundingBox(), box_b = b.boundingBox();

		if(box_a.max.y < box_b.min.y || box_b.max.y < box_b.min.y)
			return false;

		return distanceSq(	FRect(box_a.min.xz(), box_a.max.xz()),
							FRect(box_b.min.xz(), box_b.max.xz())) <= 1.0f;
	}

	EntityRef::EntityRef(Entity* node) :m_node(node) {
		if(node && node->m_first_ref) {
			m_next = node->m_first_ref;
			m_prev = m_next->m_prev;
			m_next->m_prev = this;
			m_prev->m_next = this;
		}
		else {
			m_next = this;
			m_prev = this;
		}

		if(node)	
			node->m_first_ref = this;
	}

	void EntityRef::zero() {
		m_node = nullptr;
		m_next = m_prev = nullptr;
	}

	void EntityRef::unlink() {
		if(m_next == this) {
			if(m_node)
				m_node->m_first_ref = nullptr;
		}
		if(m_next)
			m_next->m_prev = m_prev;
		if(m_prev)
			m_prev->m_next = m_next;

		if(m_node && m_node->m_first_ref == this)
			m_node->m_first_ref = m_next == this? nullptr : m_next;
		zero();
	}

	void EntityRef::link(Entity* node) {
		if(node) {
			m_node = node;

			if(node->m_first_ref) {
				m_next = node->m_first_ref;
				m_prev = m_next->m_prev;
				m_next->m_prev = this;
				m_prev->m_next = this;
			}
			else
				m_next = m_prev = node->m_first_ref = this;
		}
	}

	EntityRef::EntityRef() {
		zero();
	}

	EntityRef::EntityRef(const EntityRef& rhs) {
		zero();
		link(rhs.m_node);
	}

	EntityRef::EntityRef(EntityRef&& rhs) {
		m_node = rhs.m_node;
		m_prev = rhs.m_prev;
		m_next = rhs.m_next;
		if(m_prev)
			m_prev->m_next = this;
		if(m_next)
			m_next->m_prev = this;
		rhs.zero();
	}

	void EntityRef::operator=(const EntityRef& rhs) {
		if(this == &rhs)
			return;

		unlink();
		link(rhs.m_node);
	}

	EntityRef::~EntityRef() {
		unlink();
	}
		
	void EntityRef::save(Stream &sr) const {
		if(m_node) {
			sr.encodeInt(m_node->m_grid_index);
			sr.encodeInt(m_node->m_unique_id);
		}
		else {
			sr.encodeInt(-1);
		}

	}

	void EntityRef::load(Stream &sr) {
		World *world = Entity::world();
		i32 index = sr.decodeInt();
		unlink();

		if(index >= 0 && index < world->entityCount()) {
			int unique_id = sr.decodeInt();
			Entity *entity = world->getEntity(index);
			if(entity && entity->m_unique_id == unique_id)
				link(entity);
		}
	}

}
