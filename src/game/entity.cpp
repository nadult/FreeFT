#include "game/entity.h"
#include "gfx/sprite.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"
#include "game/world.h"

using namespace gfx;


namespace game {

	Entity::Entity(const char *sprite_name, const float3 &pos)
		:m_world(nullptr), m_to_be_removed(false) {
		m_sprite = Sprite::mgr[sprite_name];
		m_bbox = FBox(float3(0, 0, 0), (float3)m_sprite->boundingBox());
		m_dir_idx = 0;
		m_dir_angle = 0.0f;
		m_pos = (float3)pos;
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

	void Entity::addToRender(gfx::SceneRenderer &out) const {
		IRect rect;

		//TODO: do not allocate texture every frame
		PTexture spr_tex = new DTexture;
		gfx::Texture tex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir_idx, &rect);
		spr_tex->setSurface(tex);

		out.add(spr_tex, rect - m_sprite->offset(), m_pos, m_bbox);
	//	out.addBox(boundingBox(), m_world && m_world->isColliding(boundingBox())? Color::red : Color::white);
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

	void Entity::playSequence(int seq_id) {
		DASSERT(seq_id != -1);

		m_is_finished = false;
		if(seq_id != m_seq_id || !m_is_looped) {
			const vector<Sprite::Frame> &frames = (*m_sprite)[seq_id].frames;
			m_frame_id = 0;
			while(m_frame_id < (int)frames.size() && frames[m_frame_id].id < 0) {
				if(frames[m_frame_id].id <= Sprite::ev_first_specific)
					handleEventFrame(frames[m_frame_id]);
				m_frame_id++;
			}

			DASSERT(m_frame_id < (int)frames.size());
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

		const vector<Sprite::Frame> &frames = (*m_sprite)[m_seq_id].frames;
		int prev_frame = m_frame_id;
		m_frame_id++;

		while(m_frame_id < (int)frames.size() && frames[m_frame_id].id < 0) {
			const Sprite::Frame &frame = frames[m_frame_id];
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

		if(m_frame_id == (int)frames.size()) {
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

}
