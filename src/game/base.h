/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_BASE_H
#define GAME_BASE_H

#include "../base.h"
#include "sys/data_sheet.h"

namespace game {

	class Entity;
	class Tile;

	class Order;
	class POrder;

	DECLARE_ENUM(WeaponClass,
		unarmed,
		club,
		heavy,
		knife,
		minigun,
		pistol,
		rifle,
		rocket,
		smg,
		spear
	)

	DECLARE_ENUM(ArmourClassId,
		none,
		leather,
		metal,
		environmental,
		power
	)

	DECLARE_ENUM(ProjectileId,
		bullet,
		plasma,
		electric,
		laser,
		rocket
	)

	DECLARE_ENUM(DeathId,
		normal,
		big_hole,
		cut_in_half,
		electrify,
		explode,
		fire,
		melt,
		riddled
	)

	DECLARE_ENUM(EntityId,
		container,
		door,
		actor,
		item,
		projectile,
		impact
	)

	DECLARE_ENUM(TileId,
		wall,
		floor,
		object,
		stairs,
		roof,
		unknown
	)

	DECLARE_ENUM(SurfaceId,
		stone,
		gravel,
		metal,
		wood,
		water,
		snow,
		unknown
	);

	DECLARE_ENUM(Stance,
		prone,
		crouch,
		stand
	);

	DECLARE_ENUM(AttackMode,
		undefined = -1,
		single = 0,
		burst,
		thrust,
		slash,
		swing,
		throwing,
		punch,
		kick
	);
	
	namespace AttackMode {
		inline constexpr uint toFlags(Type t) { return t == undefined? 0 : 1 << t; }
	};

	namespace AttackModeFlags {
		enum Type {
			single		= toFlags(AttackMode::single),
			burst		= toFlags(AttackMode::burst),
			thrust		= toFlags(AttackMode::thrust),
			slash		= toFlags(AttackMode::slash),
			swing		= toFlags(AttackMode::swing),
			throwing	= toFlags(AttackMode::throwing),
			punch		= toFlags(AttackMode::punch),
			kick		= toFlags(AttackMode::kick)
		};

		uint fromString(const char*);

		AttackMode::Type getFirst(uint flags);
	};
	
	namespace Flags { enum Type : unsigned; };

	inline constexpr Flags::Type entityIdToFlag(EntityId::Type id) { return (Flags::Type)(1u << (4 + id)); }
	inline constexpr Flags::Type tileIdToFlag(TileId::Type id) { return (Flags::Type)(1u << (16 + id)); }

	static_assert(EntityId::count <= 12, "Flag limit reached");
	static_assert(TileId::count <= 8, "Flag limit reached");

	namespace Flags {
		enum Type :unsigned {
			//Generic flags: when testing, at least one flag must match
			static_entity	= 0x0001,
			dynamic_entity	= 0x0002, // can change position and/or bounding box

			container		= entityIdToFlag(EntityId::container),
			door			= entityIdToFlag(EntityId::door),
			actor			= entityIdToFlag(EntityId::actor),
			item			= entityIdToFlag(EntityId::item),
			projectile		= entityIdToFlag(EntityId::projectile),
			impact			= entityIdToFlag(EntityId::impact),

			entity			= 0xffff,

			wall_tile		= tileIdToFlag(TileId::wall),
			floor_tile		= tileIdToFlag(TileId::floor),
			object_tile		= tileIdToFlag(TileId::object),
			stairs_tile		= tileIdToFlag(TileId::stairs),
			roof_tile		= tileIdToFlag(TileId::roof),
			walkable_tile	= floor_tile | stairs_tile | roof_tile,

			tile			= 0xff0000,

			all				= 0xffffff,
	
			//Functional flags: when testing, all of the selected flags must match	
			visible			= 0x01000000,
			occluding		= 0x02000000,
			colliding		= 0x04000000,
		};

		inline constexpr Type operator|(Type a, Type b) { return (Type)((unsigned)a | (unsigned)b); }
		inline constexpr Type operator&(Type a, Type b) { return (Type)((unsigned)a & (unsigned)b); }
		inline constexpr Type operator~(Type a) { return (Type)(~(unsigned)a); }

		inline constexpr bool test(unsigned object_flags, Type test) {
			return (object_flags & test & 0xffffff) && ((object_flags & test & 0xff000000) == (test & 0xff000000));
		}
	
	}
	

	class SoundId {
	public:
		SoundId() :m_id(-1) { }
		SoundId(const char *sound_name);
		operator int() const { return m_id; }
		bool isValid() const { return m_id != -1; }

	protected:
		int m_id;
	};

	void loadData(bool verbose = false);

	DECLARE_ENUM(ProtoId,
		invalid = -1,

		item,
		item_weapon,
		item_armour,
		item_ammo,
		item_other,

		projectile,
		impact,
		door,
		container,
		actor,
		actor_armour
	);

	namespace ProtoId {
		enum {
			item_first = item_weapon,
			item_last = item_other
		};

		inline constexpr bool isItemId(int id) { return id >= item_first && id <= item_last; }
	};

	class ProtoIndex {
	public:
		ProtoIndex(int idx, ProtoId::Type type) :m_idx(idx), m_type(type) { validate(); }
		ProtoIndex() :m_idx(-1), m_type(ProtoId::invalid) { }
		ProtoIndex(Stream&);
		ProtoIndex(const XMLNode&);
		explicit operator bool() const { return isValid(); }

		void save(Stream&) const;
		void save(XMLNode) const;

		void validate();
		bool isValid() const { return m_type != ProtoId::invalid; }

		bool operator==(const ProtoIndex &rhs) const { return m_idx == rhs.m_idx && m_type == rhs.m_type; }

		ProtoId::Type type() const { return m_type; }
		int index() const { return m_idx; }

	protected:
		int m_idx;
		ProtoId::Type m_type;
	};

	struct Proto {
		Proto(const TupleParser&);

		virtual ~Proto() { }
		virtual void connect() { }
		virtual ProtoId::Type protoId() const = 0;
		virtual bool validProtoId(ProtoId::Type type) const { return false; }
		ProtoIndex index() const { return ProtoIndex(idx, protoId()); }

		string id;
		int idx;
			
		// Dummy items should specially handled
		// Their id is prefixed with _dummy
		// Examples:
		// dummy weapon is unarmed
		// dummy armour is unarmoured
		bool is_dummy;
	};

	template <class Type, class Base, ProtoId::Type proto_id_>
	struct ProtoImpl: public Base {
		template <class... Args>
		ProtoImpl(const Args&... args) :Base(args...) { }

		enum { proto_id = proto_id_ };
		virtual ProtoId::Type protoId() const { return proto_id_; }
		virtual bool validProtoId(ProtoId::Type type) const { return type == proto_id_ || Base::validProtoId(type); }
	};

	int countProtos(ProtoId::Type);
	ProtoIndex findProto(const string &name, ProtoId::Type id = ProtoId::invalid);	
	const Proto &getProto(ProtoIndex);
	const Proto &getProto(const string &name, ProtoId::Type id = ProtoId::invalid);

	inline const Proto &getProto(int index, ProtoId::Type type)
		{ return getProto(ProtoIndex(index, type)); }

	//TODO: store pointer instead of ProtoIndex
	template <class ProtoType>
	class ProtoRef {
	public:
		ProtoRef() { }
		ProtoRef(const char *id) :m_id(id) { }
		ProtoRef(ProtoIndex index) :m_index(index) { }

		bool isValid() const { return m_index.isValid(); }

		void connect() {
			if(m_id.empty())
				return;
			m_index = findProto(m_id, (ProtoId::Type)ProtoType::proto_id);
		}

		operator const ProtoType*() const {
		   return (const ProtoType*)(m_index.isValid()? &getProto(m_index) : nullptr);
		}
		const ProtoType* operator->() const {
			DASSERT(isValid());
			return operator const ProtoType*();
		}

		const string &id() const { return m_id; }
		ProtoIndex index() const { return m_index; }

	private:
		string m_id;
		ProtoIndex m_index;
	};

	// Object reference contains only pure indices, so they may be invalid
	// and refEntity / refTile methods in World may return null even if
	// isEntity / isTile returns true
	class ObjectRef {
	public:
		ObjectRef() :m_index(-1) { }
		explicit operator bool() const { return !isEmpty(); }

		bool isEmpty() const { return m_index == -1; }
		bool isEntity() const { return !isEmpty() && m_is_entity; }
		bool isTile() const { return !isEmpty() && !m_is_entity; }
		int index() const { return m_index; }

	private:
		ObjectRef(int index, bool is_entity) :m_index(index), m_is_entity(is_entity? 1 : 0) { }

		int m_index: 31;
		int m_is_entity: 1;
		friend class World;
		friend class EntityRef;
		friend class WorldViewer;
	};

	class Intersection {
	public:
		Intersection(ObjectRef ref = ObjectRef(), float distance = constant::inf)
			:m_ref(ref), m_distance(distance) { }
		explicit operator bool() const { return !isEmpty(); }

		operator const ObjectRef() const { return m_ref; }
		bool isEmpty() const { return m_ref.isEmpty(); }
		bool isEntity() const { return m_ref.isEntity(); }
		bool isTile() const { return m_ref.isTile(); }

		float distance() const { return m_distance; }

	private:
		ObjectRef m_ref;
		float m_distance;
	};

	struct PathPos {
		PathPos() :node_id(0), delta(0.0f) { }
		void save(Stream&) const;
		void load(Stream&);

		int node_id;
		float delta;
	};

	class Path {
	public:
		// Each node can differ from previous one only 
		// by a single component (x, y, or diagonal)
		// At least two points are needed for a valid path
		Path(const vector<int3> &nodes);
		Path() = default;

		void save(Stream&) const;
		void load(Stream&);

		// Returns true if finished
		// TODO: take current position (actual position, not computed from PathPos) into consideration
		bool follow(PathPos &pos, float step) const;
		float3 pos(const PathPos &pos) const;

		bool isValid(const PathPos&) const;
		bool isEmpty() const { return m_nodes.size() <= 1; }
		void visualize(int agent_size, gfx::SceneRenderer&) const;
		float length() const;

	private:
		vector<int3> m_nodes;
	};

}

#endif