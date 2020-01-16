// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/sprite.h"
#include "game/base.h"

namespace game {

	class World;
	class EntityRef;
	class Entity;

	using PEntity = Dynamic<Entity>;
	
	struct EntityProto: public Proto {
		EntityProto(const TupleParser&);

		string sprite_name;

		string description;

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

		bool isValid() const { return m_index != -1 && m_unique_id != -1; }
		operator ObjectRef() const { return ObjectRef(m_index, true); }
		explicit operator bool() const { return isValid(); }

		void operator=(const EntityRef &rhs) {
			m_index = rhs.m_index;
			m_unique_id = rhs.m_unique_id;
		}

		bool operator==(const EntityRef &rhs) const {
			return m_index == rhs.m_index && m_unique_id == rhs.m_unique_id;
		}

		void save(MemoryStream&) const;
		void load(MemoryStream&);
		int index() const { return m_index; }
		bool operator<(const EntityRef &rhs) const { return m_index < rhs.m_index; }
		
	private:
		EntityRef(int index, int unique_id) :m_index(index), m_unique_id(unique_id) { }

		int m_index;
		int m_unique_id;
		friend class World;
		friend class EntityWorldProxy;
	};

	//TODO: it shouldnt be here...
	class FindFilter {
	public:
		FindFilter(FlagsType flags, EntityRef ignore) :m_flags(flags), m_ignore_entity_ref(ignore) { }
		FindFilter(FlagsType flags, ObjectRef ignore) :m_flags(flags), m_ignore_object_ref(ignore) { DASSERT(ignore.isEntity()); }
		FindFilter(EntityRef ignore) :m_flags(Flags::all), m_ignore_entity_ref(ignore) { }
		FindFilter(ObjectRef ignore) :m_flags(Flags::all), m_ignore_object_ref(ignore) { DASSERT(ignore.isEntity()); }
		FindFilter(FlagsType flags = Flags::all) :m_flags(flags) { }

		FlagsType flags() const { return m_flags; }

	protected:
		FlagsType m_flags;
		EntityRef m_ignore_entity_ref;
		ObjectRef m_ignore_object_ref;
		friend class World;
	};

	class EntityWorldProxy {
	public:
		int index() const { return m_index; }
		EntityRef ref() const;
		
		void remove();
		void replicate();

	protected:
		EntityWorldProxy();
		EntityWorldProxy(MemoryStream&);
		EntityWorldProxy(const EntityWorldProxy&);
		virtual ~EntityWorldProxy();

		void save(MemoryStream&) const;

		// References will now point to the new object
		// current Entity will be destroyed
		void replaceMyself(PEntity &&new_entity);

		template <class TEntity, class ...Args>
		void addNewEntity(const float3 &pos, const Args&... args);
		void addEntity(PEntity &&new_entity);

		double timeDelta() const;
		double currentTime() const;
		float random() const;
		
		ObjectRef findAny(const FBox &box, const FindFilter &filter = FindFilter()) const;
		void findAll(vector<ObjectRef> &out, const FBox &box, const FindFilter &filter = FindFilter()) const;
		Intersection trace(const Segment3F &, const FindFilter &filter = FindFilter()) const;

		void playSound(SoundId, const float3 &pos, SoundType sound_type = SoundType::normal);
		void replicateSound(SoundId, const float3 &pos, SoundType sound_type = SoundType::normal);
		
		void onKill(EntityRef target, EntityRef killer);
	
		const FBox refBBox(ObjectRef) const;
		const Tile *refTile(ObjectRef) const;
		Entity *refEntity(ObjectRef) const;
		Entity *refEntity(EntityRef) const;
	
		template <class TEntity, class TRef> TEntity *refEntity(TRef ref);

		bool isClient() const;
		bool isServer() const;

		const World *world() const { return m_world; }

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
		Entity(const Sprite &sprite, CXmlNode);
		Entity(const Sprite &sprite, MemoryStream&);
		virtual ~Entity();
		
		virtual void save(MemoryStream&) const;
		virtual XmlNode save(XmlNode parent) const;
		
		static Entity *construct(CXmlNode node);
		static Entity *construct(MemoryStream&);

		virtual Entity *clone() const = 0;

		virtual FlagsType flags() const = 0;
		virtual EntityId typeId() const = 0;
		virtual bool renderAsOverlay() const { return false; }

		virtual void addToRender(SceneRenderer&, Color color = ColorId::white) const;
		virtual void interact(const Entity *interactor) { }
		virtual void onImpact(DamageType, float damage, const float3 &force, EntityRef source) { }

		//TODO: in some classes, some of these functions should be hidden
		// (for example setDir in Doors; dir can be changed only initially
		// after creation it makes no sense to change it
		void setPos(const float3&);
		const float3 &pos() const { return m_pos; }

		virtual const FBox boundingBox() const;
		const float3 bboxSize() const { return boundingBox().size(); }

		virtual const IRect screenRect() const;
		virtual const IRect currentScreenRect() const;

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
		short m_seq_idx, m_oseq_idx;		// normal sequence, overlay sequence
		short m_frame_idx, m_oframe_idx;	// normal frame, overlay frame
		bool m_is_seq_looped;
		bool m_is_seq_finished;
	};

	template <class Type, class ProtoType, EntityId entity_id, class Base = Entity>
	class EntityImpl: public Base
	{
	private:
		struct Initializer {
			Initializer(ProtoIndex index) {
				ASSERT(index.isValid());
				proto = static_cast<const ProtoType*>(&getProto(index));
				ASSERT(!proto->is_dummy);
				sprite = &Sprite::get(proto->sprite->index());
			}
			Initializer(const Proto &proto_) {
				DASSERT(proto_.validProtoId((ProtoId)ProtoType::proto_id));
				proto = static_cast<const ProtoType*>(&proto_);
				ASSERT(!proto->is_dummy);
				sprite = &Sprite::get(proto->sprite->index());
			}

			Initializer(CXmlNode node) :Initializer(ProtoIndex(node)) { }
			Initializer(MemoryStream &sr) :Initializer(ProtoIndex(sr)) { }

			const Sprite *sprite;
			const ProtoType *proto;
		};

		EntityImpl(const Initializer &init)
			:Base(*init.sprite), m_proto(*init.proto) { }
		EntityImpl(const Initializer &init, CXmlNode node)
			:Base(*init.sprite, node), m_proto(*init.proto) { }
		EntityImpl(const Initializer &init, MemoryStream &sr)
			:Base(*init.sprite, sr), m_proto(*init.proto) { }

	public:
		enum { type_id = (int)entity_id };
		EntityImpl(const Proto &proto) :EntityImpl(Initializer(proto)) { }
		EntityImpl(CXmlNode node) :EntityImpl(Initializer(node), node) { }
		EntityImpl(MemoryStream &sr) :EntityImpl(Initializer(sr), sr) { }

		virtual Entity *clone() const {
			return new Type(*static_cast<const Type*>(this));
		}
		virtual EntityId typeId() const {
			return entity_id;
		}
		virtual void save(MemoryStream &sr) const {
			sr << m_proto.index();
			Base::save(sr);
		}

		virtual XmlNode save(XmlNode parent) const {
			auto node = Base::save(parent);
			m_proto.index().save(node);
			return node;
		}

		const ProtoType &proto() const { return m_proto; }

	protected:
		const ProtoType &m_proto;
	};

	bool areAdjacent(const Entity&, const Entity&);

	template <class TEntity, class TRef>
	TEntity *EntityWorldProxy::refEntity(TRef ref) {
		return dynamic_cast<TEntity*>(refEntity(ref));
	}

	template <class TEntity, class ...Args>
	void EntityWorldProxy::addNewEntity(const float3 &pos, const Args&... args) {
		if(!isClient()) {
			PEntity entity(new TEntity(args...));
			entity->setPos(pos);
			addEntity(std::move(entity));
		}
	}
}
