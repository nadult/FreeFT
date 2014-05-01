/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "net/lobby.h"

namespace net {

	Address lobbyServerAddress() {
		return Address(resolveName("localhost"), 50000);
	}

	void ServerStatusChunk::save(Stream &sr) const {
		sr.pack(address.ip, address.port, game_mode);
		sr << server_name << map_name;
		sr.encodeInt(num_players);
		sr.encodeInt(max_players);
	}

	void ServerStatusChunk::load(Stream &sr) {
		sr.unpack(address.ip, address.port, game_mode);
		sr >> server_name >> map_name;
		num_players = sr.decodeInt();
		max_players = sr.decodeInt();
	}

}
