#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "base.h"
#include <memory>
#include "gfx/sprite.h"
#include "game/enums.h"

namespace gfx
{
	class SceneRenderer;
}

namespace game {

	using gfx::Sprite;
	
	class World;
	class Actor;
	class EntityRef;

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
		IRect screenRect() const;
		const float3 bboxSize() const { return m_bbox.size(); }

		float dirAngle() const { return m_dir_angle; }
		const float2 dir() const;
		const float2 actualDir() const;

		void setDir(const float2 &vector);
		void setDirAngle(float radians);

		void remove();

		bool testPixel(const int2 &screen_pos) const;

	protected:
		friend class World;
		World *m_world;
		bool m_to_be_removed;
		int m_grid_index;

	protected:
		virtual void think() { DASSERT(m_world); }
		virtual void nextFrame();
		virtual void handleFrameEvent(int sprite_event_id) { }

		virtual void onFireEvent(const int3 &projectile_offset) { }
		virtual void onHitEvent() { }
		virtual void onSoundEvent() { }
		virtual void onStepEvent(bool left_foot) { }
		virtual void onPickupEvent() { }

		void changeSprite(const char *new_name, bool update_bbox);
		void playSequence(int seq_id);
	
		// you shouldn't call playAnimation from this method	
		virtual void onAnimFinished() { }

		void setBBox(const FBox &box) { m_bbox = box; }
		int dirIdx() const { return m_dir_idx; }

		gfx::PSprite m_sprite;
		IRect m_max_screen_rect;

	private:
		float3 m_pos;
		FBox m_bbox;

		void handleEventFrame(const Sprite::Frame&);

		// Animation state
		int m_seq_id, m_frame_id;
		bool m_is_looped, m_is_finished;
		int m_dir_idx;
		float m_dir_angle;

	private:
		EntityRef *m_first_ref;
		friend class EntityRef;
	};

	class EntityRef
	{
	public:
		EntityRef();
		EntityRef(Entity* node);
		EntityRef(const EntityRef& rhs);
		EntityRef(EntityRef&& rhs);
		void operator=(const EntityRef& rhs);
		~EntityRef();
	
		// TODO: add verification (!=null) code everywhere, where EntityRef is used	
		Entity* get() const { return m_node; }
		Entity *operator->() const { return m_node; }
		Entity &operator*() const { DASSERT(m_node); return *m_node; }
		
	private:
		void zero();
		void unlink();
		// assume zeroed
		void link(Entity* node);

		Entity* m_node;
		EntityRef *m_next, *m_prev;

		friend class Entity;
	};

	bool areAdjacent(const Entity&, const Entity&);

	typedef std::unique_ptr<Entity> PEntity;

}

#endif
