// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/base.h"

namespace net {
	namespace limits {
		inline constexpr int packet_size = 1400, recv_packet_size = 2048, max_clients = 64,
							 min_nick_name_size = 4, max_nick_name_size = 14,
							 max_password_size = 10;
	}

	struct SeqNumber {
	public:
		SeqNumber() { }
		SeqNumber(int i) :value(i) { }
		operator int() const { return (int)value; }
		
		bool operator<(const SeqNumber &rhs) const {
			int a(value), b(rhs.value);
			return (a < b && b - a < 32768 ) ||
				   (a > b && a - b > 32768);
		}
		bool operator>(const SeqNumber &rhs) const { return rhs < *this; }
		bool operator==(const SeqNumber &rhs) const { return value == rhs.value; }

		const SeqNumber &operator++(int) {
			value++;
			return *this;
		}

		unsigned short value;
	};

	struct Address {
	public:
		Address() :ip(0), port(0) { }
		Address(u32 ip, u16 port) :ip(ip), port(port) { }
		Address(u16 port); // any interface

		bool isValid() const { return ip != 0 || port != 0; }
		bool operator==(const Address &rhs) const { return ip == rhs.ip && port == rhs.port; }
		bool operator<(const Address &rhs) const { return ip == rhs.ip? port < rhs.port : ip < rhs.ip; }

		const string toString() const;

		u32 ip;
		u16 port;
	};

	void encodeInt3(MemoryStream &, const int3 &value);
	const int3 decodeInt3(MemoryStream &);

	DEFINE_ENUM(LobbyChunkId, server_status, server_down, server_list,
				server_list_request, // TODO: filters
				join_request, punch_through);

	DEFINE_ENUM(RefuseReason, wrong_password, nick_already_used, server_full);

	const char *describe(RefuseReason);

	struct ServerStatusChunk {
		ServerStatusChunk() :num_players(0), max_players(0), is_passworded(false) { }

		void save(MemoryStream &sr) const;
		void load(MemoryStream &sr);

		Address address;
		string server_name;
		string map_name;
		game::GameModeId game_mode;
		int num_players, max_players;
		bool is_passworded;
	};

	struct LevelInfoChunk {
		void save(MemoryStream &sr) const;
		void load(MemoryStream &sr);

		bool isValid() const { return !map_name.empty(); }

		string map_name;
		Maybe<game::GameModeId> game_mode;
	};

}

template <> inline constexpr bool fwk::is_flat_data<net::SeqNumber> = true;
