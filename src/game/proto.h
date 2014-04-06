/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PROTO_H
#define GAME_PROTO_H

#include "../base.h"
#include "sys/data_sheet.h"

namespace game {

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
		virtual void link() { }
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

		void link() {
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

	void loadData(bool verbose = false);
}

#endif

