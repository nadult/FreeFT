/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"
#include "sys/data_sheet.h"

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
	
	namespace ItemType {	
		ProtoId::Type toProtoId(int id) {
			DASSERT(isValid(id));
			return (ProtoId::Type)(id + ProtoId::item_first);
		}
	}
	
	ItemProto::ItemProto(const TupleParser &parser) :ProtoImpl(parser) {
		name = parser("name");
		description = parser("description");
		weight = toFloat(parser("weight"));
		
		for(int n = 0; n < arraySize(seq_ids); n++) {
			if(is_dummy)
				seq_ids[n] = -1;
			else {
				seq_ids[n] = sprite->findSequence(s_seq_names[n]);
				ASSERT(seq_ids[n] != -1);
			}
		}
	}

	Item::Item(ProtoIndex index)
		:m_proto( (ASSERT(ProtoId::isItemId(index.type())), ASSERT(index.isValid()), static_cast<const ItemProto*>(&getProto(index))) ) { }
		
	const Item Item::dummy() {
		return Item(findProto("_dummy_item", ProtoId::item));
	}

	const Item Item::dummyAmmo() {
		return Item(findProto("_dummy_ammo", ProtoId::item_ammo));
	}

	const Item Item::dummyArmour() {
		return Item(findProto("_dummy_armour", ProtoId::item_armour));
	}

	const Item Item::dummyWeapon() {
		return Item(findProto("_dummy_weapon", ProtoId::item_weapon));
	}
		
	PTexture Item::guiImage(bool small, FRect &tex_rect) const {
		const Sprite &sprite = Sprite::get(m_proto->sprite->index());
		return sprite.getFrame(m_proto->seq_ids[small?2 : 1], 0, 0, tex_rect, false);
	}

	void Item::save(Stream &sr) const {
		sr << index();
	}

	ItemEntity::ItemEntity(const Item &item, int count)
			:EntityImpl(item.proto()), m_item(item), m_count(count) {
		DASSERT(count >= 1);
		playSequence(m_proto.seq_ids[0], false);
	}

	ItemEntity::ItemEntity(const XMLNode &node) :EntityImpl(node), m_item(m_proto) {
		m_count = node.intAttrib("item_count");
		playSequence(m_proto.seq_ids[0], false);
	}

	ItemEntity::ItemEntity(Stream &sr) :EntityImpl(sr), m_item(m_proto) {
		m_count = sr.decodeInt();
		ASSERT(m_count > 0);
		playSequence(m_proto.seq_ids[0], false);
	}

	void ItemEntity::save(Stream &sr) const {
		EntityImpl::save(sr);
		sr.encodeInt(m_count);
	}
	
	XMLNode ItemEntity::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		node.addAttrib("item_count", m_count);
		return node;
	}
			
		
	const FBox ItemEntity::boundingBox() const {
		float3 bbox_size = m_sprite.bboxSize();
		return FBox(0, 0, 0, bbox_size.x, 0.0f, bbox_size.z) + pos();
	}
		
	void ItemEntity::setCount(int count) {
		DASSERT(count > 0);
		m_count = count;
	}
	
	PTexture ItemEntity::guiImage(bool small, FRect &tex_rect) const {
		return m_item.guiImage(small, tex_rect);
	}

}
