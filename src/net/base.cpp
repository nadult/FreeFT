// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "net/base.h"

namespace net {

static const EnumMap<RefuseReason, const char *> s_descs = {
	{"wrong password", "nick name already used", "server full"}};

const char *describe(RefuseReason reason) { return s_descs[reason]; }

void encodeInt3(MemoryStream &sr, const int3 &value) {
	encodeInt(sr, value.x);
	encodeInt(sr, value.y);
	encodeInt(sr, value.z);
}

const int3 decodeInt3(MemoryStream &sr) {
	int3 out;
	out.x = decodeInt(sr);
	out.y = decodeInt(sr);
	out.z = decodeInt(sr);
	return out;
}

void ServerStatusChunk::save(MemoryStream &sr) const {
	sr.pack(address.ip, address.port, game_mode, is_passworded);
	sr << server_name << map_name;
	encodeInt(sr, num_players);
	encodeInt(sr, max_players);
}

void ServerStatusChunk::load(MemoryStream &sr) {
	sr.unpack(address.ip, address.port, game_mode, is_passworded);
	sr >> server_name >> map_name;
	num_players = decodeInt(sr);
	max_players = decodeInt(sr);
}

void LevelInfoChunk::save(MemoryStream &sr) const { sr << game_mode << map_name; }

void LevelInfoChunk::load(MemoryStream &sr) { sr >> game_mode >> map_name; }

}
