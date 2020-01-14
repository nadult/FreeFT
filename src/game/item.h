// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"

namespace game {

	DEFINE_ENUM(ItemType,
		weapon,
		armour,
		ammo,
		other
	);

	ProtoId toProtoId(ItemType);

	struct ItemProto: public ProtoImpl<ItemProto, EntityProto, ProtoId::item> {
		ItemProto(const TupleParser&);
		virtual ItemType itemType() const = 0;

		string description;
		string name;
		float weight;
		int seq_ids[3];
	};

	struct OtherItemProto: public ProtoImpl<OtherItemProto, ItemProto, ProtoId::other> {
		ItemType itemType() const { return ItemType::other; }
		OtherItemProto(const TupleParser &parser) :ProtoImpl(parser) { }
	};

	struct Item
	{
	public:
		Item(ProtoIndex);
		Item(const ItemProto &proto) :m_proto(&proto) { }
		Item(const Item &rhs) :m_proto(rhs.m_proto) { }
		Item(Stream &sr) :Item(ProtoIndex(sr)) { }
		Item() { *this = dummy(); }

		static const Item dummy();
		static const Item dummyAmmo();
		static const Item dummyArmour();
		static const Item dummyWeapon();

		bool operator==(const Item &rhs) const { return index() == rhs.index(); }

		ItemType type() const			{ return m_proto->itemType(); }
		bool isDummy() const				{ return m_proto->is_dummy; }
		const string &spriteName() const	{ return m_proto->sprite_name; }
		const string &name() const			{ return m_proto->name; }
		const string &id() const			{ return m_proto->id; }
		float weight() const 				{ return m_proto->weight; }

		const ItemProto &proto() const		{ return *m_proto; }
		ProtoIndex index() const			{ return m_proto->index(); }
		
		PTexture guiImage(bool small, FRect &tex_rect) const;

		void save(Stream&) const;

	protected:
		const ItemProto *m_proto;
	};

	class ItemEntity: public EntityImpl<ItemEntity, ItemProto, EntityId::item> {
	public:
		ItemEntity(Stream&);
		ItemEntity(CXmlNode);
		ItemEntity(const Item &item, int count);

		FlagsType flags() const { return Flags::item | Flags::static_entity; }

		PTexture guiImage(bool small, FRect &tex_rect) const;
		const Item &item() const { return m_item; }
		Item &item() { return m_item; }
		
		int count() const { return m_count; }
		void setCount(int count);

		virtual XmlNode save(XmlNode) const;
		virtual void save(Stream&) const;

		virtual const FBox boundingBox() const;
		
	private:
		void initialize();

		Item m_item;
		int m_count;
	};

	using PItem = shared_ptr<ItemEntity>;

}
