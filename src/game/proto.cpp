// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/proto.h"
#include "sys/data_sheet.h"

#include "game/sprite.h"
#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"
#include "game/ammo.h"
#include "game/projectile.h"
#include "game/door.h"
#include "game/container.h"
#include "game/actor.h"
#include "game/turret.h"
#include "game/character.h"

namespace game {

	Proto::Proto(const TupleParser &parser) {
		id = parser("id");
		idx = -1;
		is_dummy = id[0] == '_';
	}

	struct ProtoDef {
		std::map<string, int> map;
		int count;
		const char *table_name;
		int (*add_func)(TupleParser&);
		Proto& (*get_func)(int);
	};

	namespace {
		vector<WeaponProto> s_weapons;
		vector<ArmourProto> s_armours;
		vector<AmmoProto> s_ammo;
		vector<OtherItemProto> s_other;

		vector<ProjectileProto> s_projectiles;
		vector<ImpactProto> s_impacts;
		vector<DoorProto> s_doors;
		vector<ContainerProto> s_containers;
		vector<ActorProto> s_actors;
		vector<ActorArmourProto> s_actor_armours;
		vector<TurretProto> s_turrets;

		EnumMap<ProtoId, ProtoDef> s_protos = {
			{ std::map<string, int>(), 0, 0, 0, 0 },

#define PROTO_DEF(table_name, container) \
			{ std::map<string, int>(), 0, table_name, \
				[](TupleParser &parser) { container.emplace_back(parser); \
					return (container.back().idx = (int)container.size() - 1); }, \
				[](int idx) -> Proto& { return container[idx]; } }

			PROTO_DEF("weapons", s_weapons),
			PROTO_DEF("armours", s_armours),
			PROTO_DEF("ammo", s_ammo),
			PROTO_DEF("other_items", s_other),

			PROTO_DEF("projectiles", s_projectiles),
			PROTO_DEF("impacts", s_impacts),
			PROTO_DEF("doors", s_doors),
			PROTO_DEF("containers", s_containers),
			PROTO_DEF("actors", s_actors),
			PROTO_DEF("actor_armours", s_actor_armours),
			PROTO_DEF("turrets", s_turrets),

#undef PROTO_DEF
		};

		bool s_is_loaded = false;
	}
		
	ProtoIndex::ProtoIndex(Stream &sr) {
		sr >> m_type;
		if(m_type) {
			m_idx = decodeInt(sr);
			validate();
		}
		else
			m_idx = -1;
	}
	
	ProtoIndex::ProtoIndex(const XMLNode &node) {
		const char *proto_type = node.attrib("proto_type");
		if(strcmp(proto_type, "invalid") == 0)
			*this = ProtoIndex();
		else {
			m_type = fromString<ProtoId>(proto_type);
			m_idx = findProto(node.attrib("proto_id"), m_type).m_idx;
			if(m_idx == -1)
				CHECK_FAILED("Couldn't find proto: %s (type: %s)\n", node.attrib("proto_id"), proto_type);
			validate();
		}
	}
		
	void ProtoIndex::save(Stream &sr) const {
		sr << m_type;
		if(isValid())
			encodeInt(sr, m_idx);
	}

	void ProtoIndex::save(XMLNode node) const {
		if(isValid()) {
			const Proto &proto = getProto(*this);
			node.addAttrib("proto_type", m_type? toString(*m_type) : "invalid");
			node.addAttrib("proto_id", node.own(proto.id));
		}
		else {
			node.addAttrib("proto_type", "invalid");
		}
	}

	void ProtoIndex::validate() {
		if(m_idx != -1 || m_type) {
			if(!m_type || !validEnum(*m_type)) {
				*this = ProtoIndex();
				CHECK_FAILED("Invalid proto type: %d\n", m_type? (int)*m_type : -1);
				return;
			}

			int count = s_protos[*m_type].count;
			if(m_idx < 0 || m_idx >= count) {
				ProtoId type = *m_type;
				*this = ProtoIndex();
				CHECK_FAILED("Invalid proto index: %d (type: %s, count: %d)\n",
						m_idx, toString(type), count);
			}
		}
	}
	
	static ProtoIndex findItemProto(const string &name) {
		for(auto id : all<ProtoId>()) if(isItem(id)) {
			ProtoIndex index = findProto(name, id);
			if(index.isValid())
				return index;
		}

		return ProtoIndex();
	}

	ProtoIndex findProto(const string &name, Maybe<ProtoId> id) {
		if(!id)
			return ProtoIndex();
		if(id == ProtoId::item)
			return findItemProto(name);

		DASSERT(validEnum(*id));
		const ProtoDef &def = s_protos[*id];
		auto it = def.map.find(name);
		return it == def.map.end()? ProtoIndex() : ProtoIndex(it->second, *id);
	}
	
	int countProtos(ProtoId id) {
		return s_protos[id].count;
	}

	const Proto &getProto(ProtoIndex index) {
		DASSERT(index.isValid());
		return s_protos[index.type()].get_func(index.index());
	}
	
	const Proto &getProto(const string &name, Maybe<ProtoId> id) {
		ProtoIndex index = findProto(name, id);
		if(!index.isValid())
			CHECK_FAILED("Proto (type: %s) not found: %s", id? toString(*id) : "invalid", name.c_str());
		return getProto(index);
	}

	static void loadProtos(const char *file_name) {
		if(s_is_loaded)
			FATAL("Proto-tables have already been loaded");
		s_is_loaded = true;

		XMLDocument doc;
		Loader(file_name) >> doc;
		
		XMLNode doc_node = doc.child("office:document");
		ASSERT(doc_node);

		XMLNode body_node = doc_node.child("office:body");
		ASSERT(body_node);

		XMLNode spreadsheet_node = body_node.child("office:spreadsheet");
		ASSERT(spreadsheet_node);

		for(auto p : all<ProtoId>()) {
			ProtoDef &def = s_protos[p];
			if(!def.table_name)
				continue;

			XMLNode table_node = spreadsheet_node.child("table:table");
			while(table_node && !caseEqual(table_node.attrib("table:name"), def.table_name))
				table_node = table_node.sibling("table:table");

			if(!table_node)
				FATAL("Missing table: %s", def.table_name);

			loadDataSheet(table_node, def.map, def.add_func);
			def.count = (int)def.map.size();
		}

		for(auto p : all<ProtoId>()) {
			ProtoDef &def = s_protos[p];
			for(int n = 0; n < def.count; n++) {
				Proto &proto = s_protos[p].get_func(n);
				ON_FAIL("Error when linking proto: % (table: %)", proto.id.c_str(), def.table_name);
				proto.link();
			}
		}
	}

	void loadData(bool verbose) {
		double time;
		Sprite::initMap();

		if(verbose) {
			time = getTime();
			printf("Loading tables: ");
		}

		loadProtos("data/tables.fods");

		if(verbose) {
			int count = 0;
			for(auto p : all<ProtoId>())
				count += countProtos((ProtoId)p);
			printf(" %d rows loaded (%.0f msec)\n", count, (getTime() - time) * 1000.0);
		}
		
		CharacterClass::loadAll();
	}


}
