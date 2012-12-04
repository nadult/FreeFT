#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "base.h"
#include <memory>
#include "gfx/sprite.h"

namespace gfx
{
	class SceneRenderer;
}

namespace game {

	using gfx::Sprite;
	
	class World;
	class Actor;

	//TODO: static polimorphism where its possible, or maybe even
	// array for each type of Entity
	class Entity {
	public:
		Entity(const char *sprite_name, const int3 &pos);
		virtual ~Entity();

		// Doesn't change its bounding box
		virtual bool isStatic() const { return true; }

		virtual void addToRender(gfx::SceneRenderer&) const;
		virtual void interact(const Entity *interactor) { }

		void roundPos();
		void setPos(const float3&);
		const float3 &pos() const { return m_pos; }
		IBox boundingBox() const;
		const int3 &bboxSize() const { return m_bbox; }

		float dirAngle() const { return m_dir_angle; }
		const float2 dir() const;
		void setDir(const float2 &vector);
		void setDirAngle(float radians);

		void remove();

	protected:
		friend class World;
		World *m_world;
		bool m_to_be_removed;

	protected:
		virtual void think() { DASSERT(m_world); }
		virtual void nextFrame();
		virtual void handleFrameEvent(int sprite_event_id) { }

		virtual void onFireEvent(const int3 &projectile_offset) { }
		virtual void onHitEvent() { }
		virtual void onSoundEvent() { }
		virtual void onStepEvent(bool left_foot) { }
		virtual void onPickupEvent() { }

		void playSequence(int seq_id);
	
		// you shouldn't call playAnimation from this method	
		virtual void onAnimFinished() { }

		gfx::PSprite m_sprite;

	private:
		float3 m_pos; //TODO: int3 pos + float3 offset
		int3 m_bbox;

		void handleEventFrame(const Sprite::Frame&);

		// Animation state
		int m_seq_id, m_frame_id;
		bool m_is_looped, m_is_finished;
		int m_dir_idx;
		float m_dir_angle;
	};

	typedef std::unique_ptr<Entity> PEntity;

}

#endif
