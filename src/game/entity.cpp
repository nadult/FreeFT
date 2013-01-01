#include "game/entity.h"
#include "gfx/sprite.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include "game/world.h"
#include "sys/profiler.h"

using namespace gfx;


namespace game {

	Entity::Entity(const char *sprite_name, const float3 &pos)
		:m_world(nullptr), m_to_be_removed(false), m_grid_index(-1) {
		m_sprite = Sprite::mgr[sprite_name];
		m_max_screen_rect = m_sprite->getMaxRect();
		m_bbox = FBox(float3(0, 0, 0), (float3)m_sprite->boundingBox());
		m_dir_idx = 0;
		m_dir_angle = 0.0f;
		m_pos = (float3)pos;
		m_seq_id = -1;
		playSequence(0);
	}
	Entity::~Entity() { }

	void Entity::roundPos() {
		m_pos = (int3)(m_pos + float3(0.5f, 0, 0.5f));
	}

	void Entity::remove() {
		m_to_be_removed = true;
	}

	void Entity::setPos(const float3 &new_pos) {
		m_pos = new_pos;
	}

	FBox Entity::boundingBox() const {
		return m_bbox + pos();
	}

	IRect Entity::screenRect() const {
		return m_max_screen_rect + (int2)worldToScreen(pos());
	}

	void Entity::addToRender(gfx::SceneRenderer &out) const {
		PROFILE("Entity::addToRender");
		IRect rect = m_sprite->getRect(m_seq_id, m_frame_id, m_dir_idx);
		if(!areOverlapping(out.targetRect(), rect + (int2)worldToScreen(m_pos)))
			return;

		//TODO: do not allocate texture every frame
		PTexture spr_tex = new DTexture;
		gfx::Texture tex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir_idx);
		spr_tex->setSurface(tex);

		out.add(spr_tex, rect, m_pos, m_bbox);
		if(m_world->isColliding(boundingBox(), this))
			out.addBox(boundingBox(), Color::red);
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

	void Entity::setDirAngle(float angle) {
		m_dir_angle = angle;
		m_dir_idx = m_sprite->findDir(m_seq_id, angle);
	}

	const float2 Entity::dir() const {
		return angleToVector(dirAngle());
	}

	const float2 Entity::actualDir() const {
		int dir_count = m_sprite->dirCount(m_seq_id);
		float angle = float(m_dir_idx) * (constant::pi * 2.0f) / float(dir_count);
		return angleToVector(angle);
	}

	void Entity::setDir(const float2 &vec) {
		setDirAngle(vectorToAngle(vec));
	}

	bool areAdjacent(const Entity &a, const Entity &b) {
		FBox box_a = a.boundingBox(), box_b = b.boundingBox();

		if(box_a.max.y < box_b.min.y || box_b.max.y < box_b.min.y)
			return false;

		return distanceSq(	FRect(box_a.min.xz(), box_a.max.xz()),
							FRect(box_b.min.xz(), box_b.max.xz())) <= 1.0f;
	}

}
