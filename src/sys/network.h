/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_NETWORK_H
#define SYS_NETWORK_H

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "base.h"


namespace net {

	u32 resolveName(const char *name);
	void decomposeIp(u32 ip, u8 elems[4]);

	struct Address {
	public:
		Address() :ip(0), port(0) { }
		Address(u32 ip, u16 port) :ip(ip), port(port) { }
		Address(u16 port); // any interface

		bool isValid() const { return ip != 0 || port != 0; }
		bool operator==(const Address &rhs) const { return ip == rhs.ip && port == rhs.port; }

		const string toString() const;

		u32 ip;
		u16 port;
	};


	class Socket {
	public:
		Socket(const Address &address);
		~Socket();
		
		void operator=(const Socket&) = delete;
		Socket(const Socket&) = delete;

		void operator=(Socket&&);
		Socket(Socket&&);

		int receive(char *buffer, int buffer_size, Address &source);
		void send(const char *data, int size, const Address &target);
		void close();

	protected:
		int m_fd;
	};

	struct PacketInfo {
		PacketInfo() { }
		PacketInfo(int packet_id, int time_stamp, int client_id_, int flags_);

		i32 protocol_id;
		i32 packet_id;
		i32 time_stamp;
		i8 client_id;
		i8 flags;

		enum {
			max_size = 1470,
			min_size = sizeof(protocol_id) + sizeof(packet_id) + sizeof(time_stamp) + sizeof(client_id) + sizeof(flags),
			valid_protocol_id = 0x12345678,

			flag_need_ack = 1,
		};

		void save(Stream &sr) const;
		void load(Stream &sr);
	};

	class InPacket: public Stream {
	public:
		InPacket() :Stream(true) { }

		bool end() const { return m_pos == m_size; }
		const PacketInfo &info() const { return m_info; }
		int packetId() const { return m_info.packet_id; }
		int timeStamp() const { return m_info.time_stamp; }
		int clientId() const { return m_info.client_id; }
		int flags() const { return m_info.flags; }

	protected:
		virtual void v_load(void *ptr, int size);
		void ready(int new_size);
		friend class Host;
		
		char m_data[PacketInfo::max_size];
		PacketInfo m_info;
	};

	class OutPacket: public Stream {
	public:
		OutPacket() :Stream(false) { m_size = 0; }
		OutPacket(int packet_id, int time_stamp, int client_id, int flags);

		int spaceLeft() const { return sizeof(m_data) - m_pos; }

	protected:
		virtual void v_save(const void *ptr, int size);

		friend class Host;
		char m_data[PacketInfo::max_size];
	};

	//TODO: network data verification (so that the game won't crash under any circumstances)

	class Host {
	public:
		Host(const net::Address &address) :m_socket(address) { }

		bool receive(InPacket &packet, Address &source);
		void send(const OutPacket &packet, const Address &target);

	protected:
		net::Socket m_socket;
	};

	enum class SubPacketType: char {
		join,
		join_ack,

		leave,
		ack,

		entity_full,
		entity_delete,
		entity_update,

		actor_order,
	};

}

#endif
