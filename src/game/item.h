/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ITEM_H
#define GAME_ITEM_H

#include "game/entity.h"

namespace game {

	DECLARE_ENUM(ItemType,
		invalid = -1,
		weapon = 0,
		armour,
		ammo,
		other
	);

	namespace ItemType {
		ProtoId::Type toProtoId(int id);
		static_assert(ProtoId::item_last - ProtoId::item_first + 1 == ItemType::count, "Invalid item count in ProtoId");
	}

	struct ItemProto: public ProtoImpl<ItemProto, EntityProto, ProtoId::item> {
		ItemProto(const TupleParser&);
		virtual ItemType::Type itemType() const = 0;

		string description;
		string name;
		float weight;
		int seq_ids[3];
	};

	struct OtherItemProto: public ProtoImpl<OtherItemProto, ItemProto, ProtoId::item_other> {
		ItemType::Type itemType() const { return ItemType::other; }
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

		ItemType::Type type() const			{ return m_proto->itemType(); }
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
		ItemEntity(const XMLNode&);
		ItemEntity(const Item &item, int count);

		Flags::Type flags() const { return Flags::item | Flags::static_entity; }

		PTexture guiImage(bool small, FRect &tex_rect) const;
		const Item &item() const { return m_item; }
		Item &item() { return m_item; }
		
		int count() const { return m_count; }
		void setCount(int count);

		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;

		virtual const FBox boundingBox() const;
		
	private:
		void initialize();

		Item m_item;
		int m_count;
	};

	typedef Ptr<ItemEntity> PItem;

}



#endif
