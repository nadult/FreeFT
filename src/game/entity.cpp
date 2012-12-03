#include "game/entity.h"
#include "gfx/sprite.h"
#include "gfx/device.h"
#include "gfx/scene_renderer.h"

using namespace gfx;


namespace game {

	Entity::Entity(const char *sprite_name, const int3 &pos)
		:m_world(nullptr) {
		m_sprite = Sprite::mgr[sprite_name];
		m_bbox = m_sprite->m_bbox;
		m_dir = 0;
		m_pos = (float3)pos;
		playSequence(0);
	}
	Entity::~Entity() { }

	void Entity::roundPos() {
		m_pos = (int3)(m_pos + float3(0.5f, 0, 0.5f));
	}

	void Entity::setPos(const float3 &new_pos) {
		DASSERT(new_pos.x >= 0.0f && new_pos.y >= 0.0f && new_pos.z >= 0.0f);
		DASSERT(!isStatic());
		m_pos = new_pos;
	}

	void Entity::setDir(int new_dir) {
		//TODO: use direction vector instead of index, which can be wrong
		m_dir = new_dir;
	}

	IBox Entity::boundingBox() const {
		int3 ipos(m_pos);
		float eps = 0.0001f;
		int3 frac(m_pos.x - ipos.x > eps?1 : 0, m_pos.y - ipos.y > eps? 1 : 0, m_pos.z - ipos.z > eps? 1 : 0);
		return IBox(ipos, ipos + m_bbox + frac);
	}

	void Entity::addToRender(gfx::SceneRenderer &out) const {
		Sprite::Rect rect;

		//TODO: do not allocate texture every frame
		PTexture spr_tex = new DTexture;
		gfx::Texture tex = m_sprite->getFrame(m_anim_state.seq_id, m_anim_state.frame_id, m_dir, &rect);
		spr_tex->setSurface(tex);

		out.add(spr_tex, IRect(rect.left, rect.top, rect.right, rect.bottom) - m_sprite->m_offset, m_pos, m_bbox);
	//	out.addBox(boundingBox(), m_world && m_world->isColliding(boundingBox())? Color::red : Color::white);
	}

	void Entity::playSequence(int seq_id) {
		DASSERT(seq_id != -1);

		if(seq_id != m_anim_state.seq_id || !m_anim_state.is_looped)
			m_anim_state.frame_id = 0;
		if(seq_id != m_anim_state.seq_id) {
			m_anim_state.seq_id = seq_id;
			m_anim_state.is_looped = m_sprite->isSequenceLooped(seq_id);
		}
	}


	void Entity::animate(int frame_skip) {
		DASSERT(frame_skip);
		
		int frame_count = m_sprite->frameCount(m_anim_state.seq_id);
		int next_frame_id = m_anim_state.frame_id + frame_skip;

		bool is_finished = next_frame_id >= frame_count;
		if(is_finished)
			onAnimFinished();

		if(!is_finished || m_anim_state.is_looped)
			m_anim_state.frame_id = next_frame_id % frame_count;
	}



}
