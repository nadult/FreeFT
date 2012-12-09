#include "game/item.h"
#include "sys/xml.h"
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

	static std::map<string, PItemDesc> s_items;
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

			if(type == ItemTypeId::weapon) {
				WeaponDesc *weapon;
				item = PItemDesc(weapon = new WeaponDesc);
				weapon->projectile_type = ProjectileTypeId::fromString(node.attrib("projectile_type"));
				weapon->damage = node.floatAttrib("damage");
				weapon->projectile_speed = node.floatAttrib("projectile_speed");

			}
			else if(type == ItemTypeId::ammo) {
				AmmoDesc *ammo;
				item = PItemDesc(ammo = new AmmoDesc);
				ammo->damage_modifier = node.floatAttrib("damage_modifier");
			}
			else if(type == ItemTypeId::armour) {
				ArmourDesc *armour;
				item = PItemDesc(armour = new ArmourDesc);
				armour->damage_resistance = node.floatAttrib("damage_resistance");
			}
			else {
				ASSERT(type == ItemTypeId::other);
				item = PItemDesc(new ItemDesc);
			}

			item->name = node.attrib("name");
			item->sprite_name = node.attrib("sprite_name");
			item->description = node.attrib("description");
			item->weight = node.floatAttrib("weight");

			s_items[node.attrib("id")] = std::move(item);

			node = node.sibling("item");
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

}
