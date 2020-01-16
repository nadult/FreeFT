// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "../base.h"
#include "sys/data_sheet.h"

namespace game {

	DEFINE_ENUM(ProtoId,
		item,
		weapon,
		armour,
		ammo,
		other,

		projectile,
		impact,
		door,
		container,
		actor,
		actor_armour,
		turret
	);

	inline constexpr bool isItem(ProtoId id) {
		return isOneOf(id, ProtoId::weapon, ProtoId::armour, ProtoId::ammo, ProtoId::other);
	}

	class ProtoIndex {
	public:
		ProtoIndex(int idx, ProtoId type) :m_idx(idx), m_type(type) { validate(); }
		ProtoIndex() = default;
		ProtoIndex(MemoryStream&);
		ProtoIndex(CXmlNode);
		explicit operator bool() const { return isValid(); }

		void save(MemoryStream&) const;
		void save(XmlNode) const;

		void validate();
		bool isValid() const { return (bool)m_type; }

		bool operator==(const ProtoIndex &rhs) const { return m_idx == rhs.m_idx && m_type == rhs.m_type; }
		bool operator<(const ProtoIndex &rhs) const { return m_type == rhs.m_type? m_idx < rhs.m_idx : m_type < rhs.m_type; }

		ProtoId type() const { DASSERT(isValid()); return *m_type; }
		int index() const { return m_idx; }

	protected:
		int m_idx = -1;
		Maybe<ProtoId> m_type; // TODO: do we really want to deal with this? Just make only valid protos
	};

	struct Proto {
		Proto(const TupleParser&);

		virtual ~Proto() { }
		virtual void link() { }
		virtual ProtoId protoId() const = 0;
		virtual bool validProtoId(ProtoId type) const { return false; }
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

	template <class Type, class Base, ProtoId proto_id_>
	struct ProtoImpl: public Base {
		template <class... Args>
		ProtoImpl(const Args&... args) :Base(args...) { }

		enum { proto_id = (int)proto_id_ };
		virtual ProtoId protoId() const { return proto_id_; }
		virtual bool validProtoId(ProtoId type) const { return type == proto_id_ || Base::validProtoId(type); }
	};

	int countProtos(ProtoId);
	ProtoIndex findProto(const string &name, Maybe<ProtoId> id = none);
	const Proto &getProto(ProtoIndex);
	const Proto &getProto(const string &name, Maybe<ProtoId> id = none);

	inline const Proto &getProto(int index, ProtoId type)
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
			m_index = findProto(m_id, (ProtoId)ProtoType::proto_id);
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
