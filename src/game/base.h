/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_BASE_H
#define GAME_BASE_H

#include "../base.h"
#include "sys/data_sheet.h"

namespace game {

	DECLARE_ENUM(WeaponClassId,
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

	DECLARE_ENUM(ActorTypeId,
		male,
		female,
		ghoul,
		vault_male,
		vault_female,
		mutant,

		rad_scorpion,
		giant_rat,
		wolf,
		brahmin,
		mdc,
		sdc
	)

	DECLARE_ENUM(ProjectileTypeId,
		bullet,
		plasma,
		electric,
		laser,
		rocket
	)

	DECLARE_ENUM(DeathTypeId,
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

	DECLARE_ENUM(StanceId,
		standing,
		crouching,
		prone
	);

	DECLARE_ENUM(AttackMode,
		single,
		burst,
		thrust,
		slash,
		throwing,
		punch,
		kick,

		undefined
	);
	
	namespace AttackMode {
		inline constexpr uint toFlags(Type t) { return t == undefined? 0 : 1 << t; }
		inline int actionId(Type t) { return t == burst || t == kick? 2 : 1; }
	};

	namespace AttackModeFlags {
		enum Type {
			single		= toFlags(AttackMode::single),
			burst		= toFlags(AttackMode::burst),
			thrust		= toFlags(AttackMode::thrust),
			slash		= toFlags(AttackMode::slash),
			throwing	= toFlags(AttackMode::throwing),
		};

		uint fromString(const char*);

		AttackMode::Type getFirst(uint flags);
	};
	

	inline constexpr int tileIdToFlag(TileId::Type id) { return 1 << (16 + id); }

	//TODO: better name
	enum ColliderFlags {
		collider_none			= 0x0000,

		collider_static			= 0x0002, // updates NavigationMap when its being fully recomputed
		collider_dynamic_nv		= 0x0004, // updates NavigationMap every frame
		collider_dynamic		= 0x0008, // does not update NavigationMap

		collider_item			= 0x0100,
		collider_projectile		= 0x0200,

		collider_entities		= 0x00ffff,

		collider_tile_walls		= tileIdToFlag(TileId::wall),
		collider_tile_floors	= tileIdToFlag(TileId::floor),
		collider_tile_objects	= tileIdToFlag(TileId::object),
		collider_tile_stairs	= tileIdToFlag(TileId::stairs),
		collider_tile_roofs		= tileIdToFlag(TileId::roof),
		collider_tiles			= 0xff0000,

		collider_solid			= 0xff00ff,
		collider_all			= 0xffffff,
	};
	
	enum FunctionalFlags {
		visibility_flag			= 0x80000000,
	};
	
	inline constexpr ColliderFlags operator|(ColliderFlags a, ColliderFlags b)
			{ return (ColliderFlags)((int)a | (int)b); }

	class SoundId {
	public:
		SoundId() :m_id(-1) { }
		SoundId(const char *sound_name);
		operator int() const { return m_id; }

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

		const string &id() const { return m_id; }
		ProtoIndex index() const { return m_index; }

	private:
		string m_id;
		ProtoIndex m_index;
	};

}

#endif
