/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "game/sprite.h"
#include "game/base.h"
#include "sys/xml.h"

namespace game {

	class World;
	class Actor;
	class EntityRef;
	class Entity;

	typedef std::unique_ptr<Entity> PEntity;

	struct EntityProto: public Proto {
		EntityProto(const TupleParser&);

		string sprite_name;

		// This sprite can be only partially loaded
		// Use Sprite::get to make sure that its fully loaded
		const Sprite *sprite;
	};

	// Serializable, index-based reference to entity
	// unique_id assures that when referenced entity is destroyed,
	// refEntity will return null (instead of pointer to some new entity at that index)
	class EntityRef
	{
	public:
		EntityRef(const EntityRef &rhs) :m_index(rhs.m_index), m_unique_id(rhs.m_unique_id) { }
		EntityRef() :m_index(-1), m_unique_id(-1) { }
		bool isEmpty() const { return m_index == -1; }

		void operator=(const EntityRef &rhs) {
			m_index = rhs.m_index;
			m_unique_id = rhs.m_unique_id;
		}

		void save(Stream&) const;
		void load(Stream&);
		
	private:
		EntityRef(int index, int unique_id) :m_index(index), m_unique_id(unique_id) { }

		int m_index;
		int m_unique_id;
		friend class World;
		friend class EntityWorldProxy;
	};


	class EntityWorldProxy {
	public:
		int index() const { return m_index; }
		EntityRef makeRef();
		
		void remove();
		void replicate();

	protected:
		EntityWorldProxy();
		EntityWorldProxy(Stream&);
		EntityWorldProxy(const EntityWorldProxy&);
		virtual ~EntityWorldProxy();

		void save(Stream&) const;

		// References will now point to the new object
		// current Entity will be destroyed
		void replaceMyself(PEntity &&new_entity);
		void addEntity(PEntity &&new_entity);
		void addEntity(Entity *new_entity) { addEntity(PEntity(new_entity)); }

		double timeDelta() const;
		double currentTime() const;
		
		ObjectRef findAny(const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
		void findAll(vector<ObjectRef> &out, const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
		Intersection trace(const Segment &segment, const Entity *ignore = nullptr, int flags = collider_all) const;
	
		const FBox refBBox(ObjectRef) const;
		const Tile *refTile(ObjectRef) const;
		Entity *refEntity(ObjectRef) const;
		Entity *refEntity(EntityRef) const;

		bool isClient() const;
		bool isServer() const;
		const World *world() const { return m_world; }
		World *world() { return m_world; }

	private:
		void hook(World *world, int index);
		void unhook();
		bool isHooked() const;

		World *m_world;
		int m_index;
		int m_unique_id;
		friend class World;
		friend class EntityMap;
	};

	// TODO: check for exception safety everywhere, where Entity* is used
	class Entity: public EntityWorldProxy {
	public:
		Entity(const Sprite &sprite);
		Entity(const Sprite &sprite, const XMLNode&);
		Entity(const Sprite &sprite, Stream&);

		Entity(const Entity&);
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
		virtual void onImpact(int type, float damage) { }

		//TODO: in some classes, some of these functions should be hidden
		// (for example setDir in Doors; dir can be changed only initially
		// after creation it makes no sense to change it
		void roundPos();
		void setPos(const float3&);
		const float3 &pos() const { return m_pos; }

		virtual const FBox boundingBox() const;
		const float3 bboxSize() const { return boundingBox().size(); }

		const IRect screenRect() const;
		const IRect currentScreenRect() const;

		float dirAngle() const { return m_dir_angle; }
		float actualDirAngle() const;
		const float2 dir() const;
		const float2 actualDir() const;
		const Sprite &sprite() const { return m_sprite; }

		void setDir(const float2 &vector);
		virtual void setDirAngle(float radians);

		bool testPixel(const int2 &screen_pos) const;

		// Assumes that object is hooked to World		
		virtual void think() { }
		virtual void nextFrame();

	protected:
		virtual void handleFrameEvent(int sprite_event_id) { }

		virtual void onFireEvent(const int3 &projectile_offset) { }
		virtual void onHitEvent() { }
		virtual void onSoundEvent() { }
		virtual void onStepEvent(bool left_foot) { }
		virtual void onPickupEvent() { }

		void playSequence(int seq_id, bool handle_events = true);
	
		// you shouldn't call playAnimation from this method	
		virtual void onAnimFinished() { }

		int dirIdx() const { return m_dir_idx; }

		// When entity is moving up/down, it might overlap with some of the tiles
		virtual bool shrinkRenderedBBox() const { return false; }

		const Sprite &m_sprite;

	private:
		float3 m_pos;

		void handleEventFrame(const Sprite::Frame&);
		void resetAnimState();

		// Animation state:
		float m_dir_angle; //TODO: remove dir_angle (dir_idx should be enough)
		short m_dir_idx;
		short m_seq_idx;
		short m_frame_idx;
		bool m_is_seq_looped;
		bool m_is_seq_finished;
	};

	//TODO: collider also
	template <class Type, class ProtoType, int entity_id>
	class EntityImpl: public Entity
	{
	private:
		static_assert(entity_id >= 0 && entity_id < EntityId::count, "Wrong entity_id");

		struct Initializer {
			Initializer(ProtoIndex index) {
				ASSERT(index.isValid());
				proto = static_cast<const ProtoType*>(&getProto(index));
				ASSERT(!proto->is_dummy);
				sprite = &Sprite::get(proto->sprite->index());
			}
			Initializer(const Proto &proto_) {
				DASSERT(proto_.validProtoId((ProtoId::Type)ProtoType::proto_id));
				proto = static_cast<const ProtoType*>(&proto_);
				ASSERT(!proto->is_dummy);
				sprite = &Sprite::get(proto->sprite->index());
			}

			Initializer(const XMLNode &node) :Initializer(ProtoIndex(node)) { }
			Initializer(Stream &sr) :Initializer(ProtoIndex(sr)) { }

			const Sprite *sprite;
			const ProtoType *proto;
		};

		EntityImpl(const Initializer &init)
			:Entity(*init.sprite), m_proto(*init.proto) { }
		EntityImpl(const Initializer &init, const XMLNode &node)
			:Entity(*init.sprite, node), m_proto(*init.proto) { }
		EntityImpl(const Initializer &init, Stream &sr)
			:Entity(*init.sprite, sr), m_proto(*init.proto) { }

	public:
		EntityImpl(const Proto &proto) :EntityImpl(Initializer(proto)) { }
		EntityImpl(const XMLNode &node) :EntityImpl(Initializer(node), node) { }
		EntityImpl(Stream &sr) :EntityImpl(Initializer(sr), sr) { }

		virtual Entity *clone() const {
			return new Type(*static_cast<const Type*>(this));
		}
		virtual EntityId::Type entityType() const {
			return EntityId::Type(entity_id);
		}
		virtual void save(Stream &sr) const {
			sr << m_proto.index();
			Entity::save(sr);
		}

		virtual XMLNode save(XMLNode &parent) const {
			XMLNode node = Entity::save(parent);
			m_proto.index().save(node);
			return node;
		}

	protected:
		const ProtoType &m_proto;
	};

	bool areAdjacent(const Entity&, const Entity&);

}

#endif
