// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/item.h"
#include "game/armour.h"
#include "game/weapon.h"
#include "sys/data_sheet.h"

namespace {

const char *s_seq_names[3] = {
	"default",
	"guibig",
	"guismall",
};

const char *s_weapon_modes[3] = {"single", "burst", "both"};

}

namespace game {

ProtoId toProtoId(ItemType id) { return (ProtoId)((int)id + (int)ProtoId::item + 1); }

ItemProto::ItemProto(const TupleParser &parser) : ProtoImpl(parser) {
	name = parser("name");
	description = parser("description");
	weight = parser.get<float>("weight");

	for(int n = 0; n < arraySize(seq_ids); n++) {
		if(is_dummy)
			seq_ids[n] = -1;
		else {
			DASSERT(sprite);
			seq_ids[n] = sprite->findSequence(s_seq_names[n]);
			ASSERT(seq_ids[n] != -1);
		}
	}
}

Item::Item(ProtoIndex index)
	: m_proto((ASSERT(isItem(index.type())), ASSERT(index.isValid()),
			   static_cast<const ItemProto *>(&getProto(index)))) {}

Item Item::dummy() { return Item(findProto("_dummy_item", ProtoId::item)); }

Item Item::dummyAmmo() { return Item(findProto("_dummy_ammo", ProtoId::ammo)); }

Item Item::dummyArmour() { return Item(findProto("_dummy_armour", ProtoId::armour)); }

Item Item::dummyWeapon() { return Item(findProto("_dummy_weapon", ProtoId::weapon)); }

PVImageView Item::guiImage(bool small, FRect &tex_rect) const {
	const Sprite &sprite = Sprite::get(m_proto->sprite->index());
	return sprite.getFrame(m_proto->seq_ids[small ? 2 : 1], 0, 0, tex_rect, false);
}

void Item::save(MemoryStream &sr) const { index().save(sr); }

ItemEntity::ItemEntity(const Item &item, int count)
	: EntityImpl(item.proto()), m_item(item), m_count(count) {
	DASSERT(count >= 1);
	playSequence(m_proto.seq_ids[0], false);
}

ItemEntity::ItemEntity(CXmlNode node) : EntityImpl(node), m_item(m_proto) {
	m_count = node.attrib<int>("item_count");
	playSequence(m_proto.seq_ids[0], false);
}

ItemEntity::ItemEntity(MemoryStream &sr) : EntityImpl(sr), m_item(m_proto) {
	m_count = decodeInt(sr);
	ASSERT(m_count > 0);
	playSequence(m_proto.seq_ids[0], false);
}

void ItemEntity::save(MemoryStream &sr) const {
	EntityImpl::save(sr);
	encodeInt(sr, m_count);
}

XmlNode ItemEntity::save(XmlNode parent) const {
	auto node = EntityImpl::save(parent);
	node.addAttrib("item_count", m_count);
	return node;
}

const FBox ItemEntity::boundingBox() const {
	float3 bbox_size = (float3)m_sprite.bboxSize();
	return FBox(0, 0, 0, bbox_size.x, 0.0f, bbox_size.z) + pos();
}

void ItemEntity::setCount(int count) {
	DASSERT(count > 0);
	m_count = count;
}

PVImageView ItemEntity::guiImage(bool small, FRect &tex_rect) const {
	return m_item.guiImage(small, tex_rect);
}

}
