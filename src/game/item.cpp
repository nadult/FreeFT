#include "game/item.h"
#include <map>

static const char *s_seq_names[3] = {
	"default",
	"guibig",
	"guismall",
};

namespace game {

	Item::Item(const ItemDesc &desc, const float3 &pos)
		:Entity(desc.sprite_name.c_str(), pos), m_desc(desc) {
		m_sprite->printInfo();

		for(int n = 0; n < COUNTOF(m_seq_ids); n++) {
			m_seq_ids[n] = m_sprite->findSequence(s_seq_names[n]);
			ASSERT(m_seq_ids[n] != -1);
		}
	}

	gfx::PTexture Item::guiImage(bool small) const {
		const gfx::Texture &tex = m_sprite->getFrame(m_seq_ids[small?2 : 1], 0, 0);
		gfx::PTexture out = new gfx::DTexture;
		out->setSurface(tex);
		return out;
	}

	namespace ItemType {

		static const char *s_strings[count] = {
			"weapon",
			"ammo",
			"armour",
			"other",
		};

		const char *toString(Type value) { return genericToString<Type, count>(value, s_strings); }
		Type fromString(const char *str) { return genericFromString<Type, count>(str, s_strings); }
	};

	static std::map<string, PItemDesc> s_items;
	static bool s_are_items_loaded = false;

	void ItemDesc::loadItems() {
		if(s_are_items_loaded)
			return;

		XMLDocument doc;
		loadXMLDocument("data/items.xml", doc);

		XMLNode *node = doc.first_node("item");
		while(node) {
			ItemType::Type type = ItemType::fromString(getStringAttribute(node, "type"));
			std::unique_ptr<ItemDesc> item;

			if(type == ItemType::weapon) {
				WeaponDesc *weapon;
				item = PItemDesc(weapon = new WeaponDesc);
				weapon->projectile_type = ProjectileType::fromString(getStringAttribute(node, "projectile_type"));
				weapon->damage = getFloatAttribute(node, "damage");
				weapon->projectile_speed = getFloatAttribute(node, "projectile_speed");

			}
			else if(type == ItemType::ammo) {
				AmmoDesc *ammo;
				item = PItemDesc(ammo = new AmmoDesc);
				ammo->damage_modifier = getFloatAttribute(node, "damage_modifier");
			}
			else if(type == ItemType::armour) {
				ArmourDesc *armour;
				item = PItemDesc(armour = new ArmourDesc);
				armour->damage_resistance = getFloatAttribute(node, "damage_resistance");
			}
			else {
				ASSERT(type == ItemType::other);
				item = PItemDesc(new ItemDesc);
			}

			//TODO: better error handling when loading XML files
			const char *id = getStringAttribute(node, "id");
			const char *name = getStringAttribute(node, "name");
			const char *sprite_name = getStringAttribute(node, "sprite_name");
			const char *description = getStringAttribute(node, "description");
			ASSERT(id && name && sprite_name && description);
			item->name = name;
			item->sprite_name = sprite_name;
			item->description = description;
			item->weight = getFloatAttribute(node, "weight");

			s_items[id] = std::move(item);

			node = node->next_sibling("item");
		}

		s_are_items_loaded = true;
	}

	const ItemDesc *ItemDesc::find(const char *name) {
		DASSERT(s_are_items_loaded);

		auto it = s_items.find(name);
		if(it != s_items.end())
			return it->second.get();
		return nullptr;
	}

	int Inventory::findItem(const ItemDesc *item) {
		for(int n = 0; n < size(); n++)
			if(m_entries[n].item == item)
				return n;
		return -1;
	}

	int Inventory::addItem(const ItemDesc *item, int count) {
		DASSERT(item && count >= 0);
		if(!count)
			return;

		int entry_id = findItem(item);
		if(entry_id != -1) {
			m_entries[entry_id].count += count;
			return entry_id;
		}

		m_entries.push_back(Entry{item, count});
		return size() - 1;
	}

	void Inventory::remove(int entry_id, int count) {
		DASSERT(entry_id >= 0 && entry_id < size());
		DASSERT(count >= 0);

		Entry &entry = m_entries[entry_id];
		entry.count -= count;
		if(entry.count <= 0) {
			m_entries[entry_id] = m_entries.back();
			m_entries.pop_back();
		}
	}

	float Inventory::weight() const {
		double sum = 0.0;
		for(int n = 0; n < size(); n++)
			sum += double(m_entries[n].item->weight) * double(m_entries[n].count);
		return float(sum);
	}

}
