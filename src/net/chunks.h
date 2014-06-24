/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_CHUNKS_H
#define NET_CHUNKS_H

#include "net/socket.h"
#include "game/base.h"
#include "game/entity.h"

namespace net {

	Address lobbyServerAddress();

	DECLARE_ENUM(LobbyChunkId,
		server_status,
		server_down,
		server_list,
		server_list_request, // TODO: filters
		join_request
	);

	struct ServerStatusChunk {
		ServerStatusChunk() :num_players(0), max_players(0), is_passworded(false) { }

		void save(Stream &sr) const;
		void load(Stream &sr);

		Address address;
		string server_name;
		string map_name;
		game::GameModeId::Type game_mode;
		int num_players, max_players;
		bool is_passworded;
	};

	struct LevelInfoChunk {
		void save(Stream &sr) const;
		void load(Stream &sr);

		bool isValid() const { return !map_name.empty(); }

		string map_name;
		game::GameModeId::Type game_mode;
		game::EntityRef actor_ref;
	};

}

#endif
