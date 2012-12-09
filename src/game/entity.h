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

	enum ColliderFlags {
		collider_none		= 0,
		collider_tiles		= 1,

		collider_static		= 2, // updates NavigationMap when its being fully recomputed
		collider_dynamic_nv	= 4, // updates NavigationMap every frame
		collider_dynamic	= 8, // does not update NavigationMap

		collider_entities = collider_static | collider_dynamic | collider_dynamic_nv,

		collider_all		= 0xffffffff,
	};

	enum EntityFlags {
		entity_container	= 1,
		entity_door			= 2,
		entity_actor		= 4,
		entity_item			= 8,
		entity_projectile	= 16,
		entity_impact		= 32,
	};

	inline ColliderFlags operator|(ColliderFlags a, ColliderFlags b) { return (ColliderFlags)((int)a | (int)b); }

	//TODO: static polimorphism where its possible, or maybe even
	// array for each type of Entity
	class Entity {
	public:
		Entity(const char *sprite_name, const float3 &pos);
		virtual ~Entity();

		virtual ColliderFlags colliderType() const = 0;
		virtual EntityFlags entityType() const = 0;

		virtual void addToRender(gfx::SceneRenderer&) const;
		virtual void interact(const Entity *interactor) { }

		//TODO: in some classes, some of these functions should be hidden
		// (for example setDir in Doors; dir can be changed only initially
		// after creation it makes no sense to change it
		void roundPos();
		void setPos(const float3&);
		const float3 &pos() const { return m_pos; }
		FBox boundingBox() const;
		const float3 bboxSize() const { return m_bbox.size(); }

		float dirAngle() const { return m_dir_angle; }
		const float2 dir() const;
		const float2 actualDir() const;

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

		void setBBox(const FBox &box) { m_bbox = box; }
		int dirIdx() const { return m_dir_idx; }

		gfx::PSprite m_sprite;

	private:
		float3 m_pos;
		FBox m_bbox;

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
