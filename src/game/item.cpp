/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/item.h"
#include "game/sounds.h"
#include "sys/xml.h"
#include <map>

namespace {

	const char *s_seq_names[3] = {
		"default",
		"guibig",
		"guismall",
	};

}

namespace game {
		
	void WeaponDesc::loadParams(XMLNode &node) {
		projectile_type_id = ProjectileTypeId::fromString(node.attrib("projectile_type"));
		damage = node.floatAttrib("damage");
		projectile_speed = node.floatAttrib("projectile_speed");
		class_id = WeaponClassId::fromString(node.attrib("class"));

		const char *sound_prefix = node.attrib("sound_prefix");
		for(int n = 0; n < WeaponSoundId::count; n++)
			sound_ids[n] = getWeaponSoundId(sound_prefix, (WeaponSoundId::Type)n);
	}

	void AmmoDesc::loadParams(XMLNode &node) {
		damage_modifier = node.floatAttrib("damage_modifier");
	}
	
	void ArmourDesc::loadParams(XMLNode &node) {
		damage_resistance = node.floatAttrib("damage_resistance");
		class_id = ArmourClassId::fromString(node.attrib("class"));
	}

	ItemEntity::ItemEntity(const Item &item, const float3 &pos) :Entity(item.spriteName()) {
		initialize(item);
		setPos(pos);
	}

	ItemEntity::ItemEntity(const XMLNode &node) :Entity(node) {
		const ItemDesc *desc = ItemDesc::find(node.attrib("item_desc"));
		if(!desc)
			THROW("Unknown item id: %s\n", node.attrib("item_desc"));
		initialize(Item(desc));
	}

	ItemEntity::ItemEntity(Stream &sr) :Entity(sr) {
		Item item;
		sr >> item;
		initialize(item);
	}

	void ItemEntity::save(Stream &sr) const {
		Entity::save(sr);
		sr << m_item;
	}
	
	XMLNode ItemEntity::save(XMLNode &parent) const {
		XMLNode node = Entity::save(parent);
		node.addAttrib("item_desc", node.own(m_item.desc()->id.c_str()));
		return node;
	}
			
	void ItemEntity::initialize(const Item &item) {
		m_item = item;
		ASSERT(item.isValid());

		setBBox(FBox(float3(0.0f, 0.0f, 0.0f), asXZY(bboxSize().xz(), 0.0f)));

		for(int n = 0; n < COUNTOF(m_seq_ids); n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			ASSERT(m_seq_ids[n] != -1);
		}
	}
	
	Entity *ItemEntity::clone() const {
		return new ItemEntity(*this);
	}

	gfx::PTexture ItemEntity::guiImage(bool small, FRect &tex_rect) const {
		return m_sprite->getFrame(m_seq_ids[small?2 : 1], 0, 0, tex_rect);
	}

	static std::vector<PItemDesc> s_items;
	static std::map<string, int> s_item_map;
	static bool s_are_items_loaded = false;

	void ItemDesc::loadItems() {
		if(s_are_items_loaded)
			return;

		XMLDocument doc;
		doc.load("data/items.xml");

		XMLNode node = doc.child("item");
		while(node) {
			ItemTypeId::Type type = ItemTypeId::fromString(node.attrib("type"));
			std::unique_ptr<ItemDesc> item;
			const char *id = nullptr;

			if(type == ItemTypeId::weapon)
				item = PItemDesc(new WeaponDesc);
			else if(type == ItemTypeId::ammo)
				item = PItemDesc(new AmmoDesc);
			else if(type == ItemTypeId::armour)
				item = PItemDesc(new ArmourDesc);
			else {
				ASSERT(type == ItemTypeId::other);
				item = PItemDesc(new ItemDesc);
			}

			try {
				id = node.attrib("id");
				item->id = id;
				item->name = node.attrib("name");
				item->sprite_name = node.attrib("sprite_name");
				item->description = node.attrib("description");
				item->weight = node.floatAttrib("weight");

				if(type == ItemTypeId::weapon) {
					WeaponDesc *weapon = static_cast<WeaponDesc*>(item.get());
					weapon->loadParams(node);
				}
				else if(type == ItemTypeId::ammo) {
					AmmoDesc *ammo = static_cast<AmmoDesc*>(item.get());
					ammo->loadParams(node);
				}
				else if(type == ItemTypeId::armour) {
					ArmourDesc *armour = static_cast<ArmourDesc*>(item.get());
					armour->loadParams(node);
				}

				s_items.push_back(std::move(item));
				s_item_map[id] = (int)s_items.size() - 1;
			}
			catch(const Exception &ex) {
				THROW("Error while parsing item with id: %s, type: %s\n%s",
						id? id : "unknown", ItemTypeId::toString(type), ex.what());
			}

			node = node.sibling("item");
		}

		s_are_items_loaded = true;
	}

	const ItemDesc *ItemDesc::find(const char *name) {
		DASSERT(s_are_items_loaded);

		auto it = s_item_map.find(name);
		if(it != s_item_map.end())
			return s_items[it->second].get();
		return nullptr;
	}
		
	const ItemDesc *ItemDesc::get(int id) {
		return s_items[id].get();
	}

	int ItemDesc::count() {
		return s_items.size();
	}

	void ItemDesc::initialize(ItemParameter *params) const {
		for(int n = 0; n < param_count; n++)
			params[n].i = 0;
	}
		
	Item::Item(const ItemDesc *desc) :m_desc(desc) {
		if(m_desc)
			m_desc->initialize(m_params);
	}
		
	float Item::weight() const {
		DASSERT(isValid());
		return m_desc->weight;
	}

	ItemTypeId::Type Item::typeId() const {
		return isValid()? m_desc->type() : ItemTypeId::invalid;
	}

	const char *Item::spriteName() const {
		DASSERT(isValid());
		return m_desc->sprite_name.c_str();
	}

	const char *Item::name() const {
		DASSERT(isValid());
		return m_desc->name.c_str();
	}

	bool Item::operator==(const Item &rhs) const {
		if(m_desc != rhs.m_desc)
			return false;
		return memcmp(m_params, rhs.m_params, sizeof(m_params)) == 0;
	}

	static_assert(ItemDesc::param_count <= 8, "");

	void Item::load(Stream &sr) {
		char item_name[256];
		sr.loadString(item_name, sizeof(item_name));

		if(item_name[0] == 0) {
			memset(m_params, 0, sizeof(m_params));
		}
		else {
			m_desc = ItemDesc::find(item_name);
			if(!m_desc)
				THROW("Unknown item id: %s\n", item_name);

			u8 param_bits = 0;
			sr >> param_bits;

			//TODO: portability
			for(int n = 0; n < ItemDesc::param_count; n++)
				if(param_bits & (1 << n))
					sr >> m_params[n].i;
				else
					m_params[n].i = 0;
		}
	}

	void Item::save(Stream &sr) const {
		if(desc()) {
			sr.saveString(desc()->id.c_str());

			u8 param_bits = 0;
			for(int n = 0; n < ItemDesc::param_count; n++)
				if(m_params[n].i)
					param_bits |= (1 << n);
			sr << param_bits;
			for(int n = 0; n < ItemDesc::param_count; n++)
				if(m_params[n].i)
					sr << m_params[n].i;

		}
		else {
			sr.saveString("");
		}
	}


	Weapon::Weapon(const Item &item) :Item(item) {
		if(isValid())
			DASSERT(typeId() == ItemTypeId::weapon);
	}
	
	Armour::Armour(const Item &item) :Item(item) {
		if(isValid())
			DASSERT(typeId() == ItemTypeId::armour);
	}

	ProjectileTypeId::Type Weapon::projectileTypeId() const {
		DASSERT(isValid());
		return desc()->projectile_type_id; 
	}
	
	float Weapon::projectileSpeed() const {
		DASSERT(isValid());
		return desc()->projectile_speed; 
	}

	WeaponClassId::Type Weapon::classId() const {
		DASSERT(isValid());
		return desc()->class_id; 
	}

	ArmourClassId::Type Armour::classId() const {
		DASSERT(isValid());
		return desc()->class_id; 
	}

}
