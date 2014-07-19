/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_BASE_H
#define NET_BASE_H

#include "game/base.h"

namespace net {

	namespace limits {

		enum {
			packet_size = 1400,

			max_clients = 64,
			min_nick_name_size = 4,
			max_nick_name_size = 14,
			max_password_size = 10,
		};

	}

	class TempPacket: public Stream {
	public:
		TempPacket() :Stream(false) { m_size = 0; }

		const char *data() const { return m_data; }
		int spaceLeft() const { return sizeof(m_data) - m_pos; }

	protected:
		virtual void v_save(const void *ptr, int size) final;

		char m_data[limits::packet_size];
	};

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

	void encodeInt3(Stream&, const int3 &value);
	const int3 decodeInt3(Stream&);

	DECLARE_ENUM(LobbyChunkId,
		server_status,
		server_down,
		server_list,
		server_list_request, // TODO: filters
		join_request
	);

	DECLARE_ENUM(RefuseReason,
		wrong_password,
		nick_already_used,
		server_full
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
	};

}

SERIALIZE_AS_POD(net::SeqNumber)

#endif
