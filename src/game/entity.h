/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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
	class Entity {
	public:
		Entity(const char *sprite_name, const float3 &pos);
		virtual ~Entity();

		virtual ColliderFlags colliderType() const = 0;
		virtual EntityId::Type entityType() const = 0;

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
		friend class EntityMap;
		World *m_world;
		bool m_to_be_removed;
		mutable int m_grid_index;

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

		// When entity is moving up/down, it might overlap with some of the tiles
		virtual bool shrinkRenderedBBox() const { return false; }

		PSprite m_sprite;
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
