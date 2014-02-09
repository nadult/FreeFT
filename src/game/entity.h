/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "base.h"
#include <memory>
#include "game/sprite.h"
#include "game/enums.h"


namespace game {

	class World;
	class Actor;
	class EntityRef;

	//TODO: static polimorphism where its possible, or maybe even
	// array for each subtype of Entity class
	// TODO: check for exception safety everywhere, where Entity* is used
	class Entity {
	protected:
		Entity();
		void initialize(const char *sprite_name);
		void saveEntityParams(Stream&) const;
		void loadEntityParams(Stream&);

	public:
		Entity(const char *sprite_name);
		Entity(const XMLNode&);
		Entity(Stream&);
		Entity(const Entity&);
		void operator=(const Entity&);
		virtual ~Entity();
		
		virtual void save(Stream&) const;
		virtual XMLNode save(XMLNode& parent) const;
		
		static Entity *construct(const XMLNode &node);
		static Entity *construct(Stream&);

		virtual Entity *clone() const = 0;

		virtual ColliderFlags colliderType() const = 0;
		virtual EntityId::Type entityType() const = 0;

		virtual void addToRender(gfx::SceneRenderer&) const;
		virtual void interact(const Entity *interactor) { }
		virtual void onImpact(ProjectileTypeId::Type projectile_type, float damage) { }

		//TODO: in some classes, some of these functions should be hidden
		// (for example setDir in Doors; dir can be changed only initially
		// after creation it makes no sense to change it
		void roundPos();
		void setPos(const float3&);
		const float3 &pos() const { return m_pos; }
		const FBox boundingBox() const;
		const IRect screenRect() const;
		const IRect currentScreenRect() const;
		const float3 bboxSize() const { return m_bbox.size(); }

		int id() const { return m_grid_index; }
		float dirAngle() const { return m_dir_angle; }
		const float2 dir() const;
		const float2 actualDir() const;
		PSprite sprite() const { return m_sprite; }

		void setDir(const float2 &vector);
		virtual void setDirAngle(float radians);

		void remove();

		bool testPixel(const int2 &screen_pos) const;

		static World *world();

	protected:
		friend class World;
		friend class EntityMap;
		bool m_to_be_removed;
		mutable int m_grid_index;
		int m_unique_id;

	protected:
		virtual void think() { }
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

		// When entity is moving up/down, it might overlap with some of the tiles
		virtual bool shrinkRenderedBBox() const { return false; }

		PSprite m_sprite;
		IRect m_max_screen_rect;

	protected: //private: //TODO: these should be private (probably)
		float3 m_pos;
		FBox m_bbox;

		void handleEventFrame(const Sprite::Frame&);

		// Animation state
		int m_seq_id, m_frame_id;
		bool m_is_looped, m_is_finished;
		int m_dir_idx;
		float m_dir_angle; //TODO: initially its equal to -nan

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

		//TODO: serialization of entity_ref	
		// TODO: add verification (!=null) code everywhere, where EntityRef is used	
		Entity* get() const { return m_node; }
		Entity *operator->() const { return m_node; }
		Entity &operator*() const { DASSERT(m_node); return *m_node; }

		void save(Stream&) const;
		void load(Stream&);
		
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
