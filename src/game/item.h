/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ITEM_H
#define GAME_ITEM_H

#include "game/entity.h"
#include "sys/data_sheet.h"
#include "gfx/device.h"

namespace game {

	DECLARE_ENUM(ItemType,
		invalid = -1,
		weapon = 0,
		armour,
		ammo,
		other
	);

	class ItemIndex {
	public:
		ItemIndex(int idx, ItemType::Type type);
		ItemIndex() :m_idx(-1), m_type(ItemType::invalid) { }

		void save(Stream&) const;
		void load(Stream&);
		void validate();
		bool isValid() const { return m_type != ItemType::invalid; }

		bool operator==(const ItemIndex &rhs) const { return m_idx == rhs.m_idx && m_type == rhs.m_type; }

		ItemType::Type type() const { return m_type; }
		int index() const { return m_idx; }

	protected:
		int m_idx;
		ItemType::Type m_type;
		friend class Item;
	};

	struct ItemDesc: public Tuple {
		enum { param_count = 2 };

		virtual ~ItemDesc() { }
		ItemDesc(const TupleParser&);

		virtual ItemType::Type type() const = 0;

		string description;
		string name;
		string sprite_name; //TODO: add sprite_map as well, identify sprites with id-s, just like with sounds
		float weight;

		// Dummy items should specially handled
		// Examples:
		// dummy weapon is unarmed
		// dummy armour is unarmoured
		bool is_dummy;
	};

	struct OtherItemDesc: public ItemDesc, TupleImpl<OtherItemDesc> {
		ItemType::Type type() const { return ItemType::other; }
		OtherItemDesc(const TupleParser &parser) :ItemDesc(parser) { }
	};

	struct AmmoDesc: public ItemDesc, TupleImpl<AmmoDesc> {
		ItemType::Type type() const { return ItemType::ammo; }
		AmmoDesc(const TupleParser&);

		string class_id;
		float damage_multiplier;
	};

	struct Item
	{
	public:
		Item(const ItemDesc &desc) :m_desc(&desc) { }
		Item(ItemIndex index) :m_desc(&get(index)) { }
		Item(int idx, ItemType::Type type) :Item(ItemIndex(idx, type)) { }
		Item(const string &name, ItemType::Type type) :Item(find(name, type), type) { }
		Item(const Item &rhs) :m_desc(rhs.m_desc) { }
		Item(Stream&);
		Item() { *this = dummyItem(); }

		static const ItemIndex find(const string &name);
		static int find(const string &name, ItemType::Type type);
		static int count(ItemType::Type type);
		static const ItemDesc &get(ItemIndex);
		static const Item dummyItem();

		bool operator==(const Item &rhs) const { return index() == rhs.index(); }
		bool operator!=(const Item &rhs) const { return !operator==(rhs); }

		ItemType::Type type() const			{ return m_desc->type(); }
		bool isDummy() const				{ return m_desc->is_dummy; }
		const string &spriteName() const	{ return m_desc->sprite_name; }
		const string &name() const			{ return m_desc->name; }
		const string &id() const			{ return m_desc->id; }
		float weight() const 				{ return m_desc->weight; }

		const ItemDesc &desc() const		{ return *m_desc; }
		const ItemIndex index() const		{ return ItemIndex(m_desc->idx, m_desc->type()); }

		void save(Stream&) const;

	protected:
		const ItemDesc *m_desc;
	};

	class ItemEntity: public Entity {
	public:
		ItemEntity(Stream&);
		ItemEntity(const XMLNode&);
		ItemEntity(const Item &item, int count, const float3 &pos);

		ColliderFlags colliderType() const { return collider_item; }
		virtual EntityId::Type entityType() const { return EntityId::item; }
		virtual Entity *clone() const;

		gfx::PTexture guiImage(bool small, FRect &tex_image) const;
		const Item &item() const { return m_item; }
		Item &item() { return m_item; }
		
		int count() const { return m_count; }
		void setCount(int count);

		virtual XMLNode save(XMLNode&) const;
		virtual void save(Stream&) const;
		
	private:
		void initialize();

		int m_seq_ids[3];
		Item m_item;
		int m_count;
	};

	typedef Ptr<ItemEntity> PItem;

}



#endif
