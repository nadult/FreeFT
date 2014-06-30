/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "net/base.h"

namespace net {

	Address lobbyServerAddress() {
		return Address(resolveName("localhost"), 50000);
		return Address(resolveName("89.74.58.32"), 50000);
	}

	void ServerStatusChunk::save(Stream &sr) const {
		sr.pack(address.ip, address.port, game_mode, is_passworded);
		sr << server_name << map_name;
		sr.encodeInt(num_players);
		sr.encodeInt(max_players);
	}

	void ServerStatusChunk::load(Stream &sr) {
		sr.unpack(address.ip, address.port, game_mode, is_passworded);
		sr >> server_name >> map_name;
		num_players = sr.decodeInt();
		max_players = sr.decodeInt();
	}


	void LevelInfoChunk::save(Stream &sr) const {
		sr << game_mode << map_name;
	}

	void LevelInfoChunk::load(Stream &sr) {
		sr >> game_mode >> map_name;
	}

}
