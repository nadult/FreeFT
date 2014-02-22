/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"
#include "sys/xml.h"

namespace {

	const char *s_seq_names[3] = {
		"default",
		"guibig",
		"guismall",
	};

	const char *s_weapon_modes[3] = {
		"single",
		"burst",
		"both"
	};

}

namespace game {
	
	DEFINE_ENUM(ItemType,
		"weapon",
		"armour",
		"ammo",
		"other"
	);

	
	AmmoDesc::AmmoDesc(const TupleParser &parser) :ItemDesc(parser) {
		damage_multiplier = toFloat(parser("damage_multiplier"));
		class_id = parser("class_id");
	}


	ItemDesc::ItemDesc(const TupleParser &parser) :Tuple(parser) {
		sprite_name = parser("sprite_name");

		name = parser("name");
		description = parser("description");
		weight = toFloat(parser("weight"));

		is_dummy = id.find("_dummy") == 0;
		if(!is_dummy)
			ASSERT(!sprite_name.empty());
	}

	namespace {

		typedef int (*find_func)(const string&);
		typedef int (*count_func)();
		typedef const ItemDesc* (*get_func)(int);

		static find_func s_find_funcs[ItemType::count] = {
			WeaponDesc::find,
			ArmourDesc::find,
			AmmoDesc::find,
			OtherItemDesc::find
		};

		static count_func s_count_funcs[ItemType::count] = {
			WeaponDesc::count,
			ArmourDesc::count,
			AmmoDesc::count,
			OtherItemDesc::count
		};

		static get_func s_get_funcs[ItemType::count] = {
			[](int idx) { return static_cast<const ItemDesc*>(&WeaponDesc::get(idx)); },
			[](int idx) { return static_cast<const ItemDesc*>(&ArmourDesc::get(idx)); },
			[](int idx) { return static_cast<const ItemDesc*>(&AmmoDesc::get(idx)); },
			[](int idx) { return static_cast<const ItemDesc*>(&OtherItemDesc::get(idx)); }
		};

	}

	ItemIndex::ItemIndex(int idx, ItemType::Type type) :m_idx(idx), m_type(type) {
		validate();
	}

	void ItemIndex::validate() {
		if(m_idx != -1 || m_type == ItemType::invalid) {
			if(!ItemType::isValid(m_type)) {
				*this = ItemIndex();
				THROW("Invalid item type: %d\n", (int)m_type);
				return;
			}

			int count = s_count_funcs[m_type]();
			if(m_idx < 0 || m_idx >= count) {
				ItemType::Type type = m_type;
				*this = ItemIndex();
				THROW("Invalid item index: %d (type: %s, count: %d)\n",
						m_idx, ItemType::toString(type), count);
			}
		}
	}

	void ItemIndex::save(Stream &sr) const {
		if(isValid()) {
			sr << m_type;
			sr.encodeInt(m_idx);
		}
		else {
			sr << ItemType::invalid;
		}
	}
		
	void ItemIndex::load(Stream &sr) {
		sr >> m_type;
		if(ItemType::isValid(m_type)) {
			m_idx = sr.decodeInt();
			validate();
		}
		else
			m_idx = -1;
	}

	static const ItemDesc &loadDesc(Stream &sr) {
		ItemIndex index;
		sr >> index;
		return Item::get(index);
	}

	Item::Item(Stream &sr) :m_desc(&loadDesc(sr)) { }
	
	int Item::find(const string &name, ItemType::Type type) {
		DASSERT(ItemType::isValid(type));
		return s_find_funcs[type](name);
	}

	const ItemIndex Item::find(const string &name) {
		for(int n = 0; n < ItemType::count; n++) {
			int idx = find(name, (ItemType::Type)n);
			if(idx != -1)
				return ItemIndex(idx, (ItemType::Type)n);
		}

		return ItemIndex();
	}

	int Item::count(ItemType::Type type) {
		DASSERT(ItemType::isValid(type));
		return s_count_funcs[type]();
	}
		
	const ItemDesc &Item::get(ItemIndex index) {
		DASSERT(index.isValid());
		return *s_get_funcs[index.m_type](index.m_idx);
	}

	const Item Item::dummyItem() {
		return Item(find("_dummy_item", ItemType::other), ItemType::other);
	}

	void Item::save(Stream &sr) const {
		sr << index();
	}


	ItemEntity::ItemEntity(const Item &item, int count, const float3 &pos)
			:Entity(item.spriteName()), m_item(item), m_count(count) {
		DASSERT(count >= 1);
		initialize();
		setPos(pos);
	}

	ItemEntity::ItemEntity(const XMLNode &node) :Entity(node),
   		m_item(node.attrib("item_desc"), ItemType::fromString(node.attrib("item_type"))) {
		m_count = node.intAttrib("count");
		initialize();
	}

	ItemEntity::ItemEntity(Stream &sr) :Entity(sr), m_item(sr) {
		m_count = sr.decodeInt();
		ASSERT(m_count > 0);
		initialize();
	}

	void ItemEntity::save(Stream &sr) const {
		Entity::save(sr);
		sr << m_item;
		sr.encodeInt(m_count);
	}
	
	XMLNode ItemEntity::save(XMLNode &parent) const {
		XMLNode node = Entity::save(parent);
		node.addAttrib("item_desc", node.own(m_item.id().c_str()));
		node.addAttrib("item_type", ItemType::toString(m_item.type()));
		node.addAttrib("count", m_count);
		return node;
	}
			
	void ItemEntity::initialize() {
		setBBox(FBox(float3(0.0f, 0.0f, 0.0f), asXZY(bboxSize().xz(), 0.0f)));

		for(int n = 0; n < COUNTOF(m_seq_ids); n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			ASSERT(m_seq_ids[n] != -1);
		}
	}
		
	void ItemEntity::setCount(int count) {
		DASSERT(count > 0);
		m_count = count;
	}
	
	Entity *ItemEntity::clone() const {
		return new ItemEntity(*this);
	}

	gfx::PTexture ItemEntity::guiImage(bool small, FRect &tex_rect) const {
		return m_sprite->getFrame(m_seq_ids[small?2 : 1], 0, 0, tex_rect);
	}

}
