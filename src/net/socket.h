// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "net/base.h"

namespace net {

	class InPacket;
	class OutPacket;
	
	u32 resolveName(const char *name);
	void decomposeIp(u32 ip, u8 elems[4]);
	int randomPort();
	const Address lobbyServerAddress();

	class Socket {
	public:
		Socket(const Address &address);
		Socket() :m_fd(0) { }
		~Socket();
		
		void operator=(const Socket&) = delete;
		Socket(const Socket&) = delete;

		void operator=(Socket&&);
		Socket(Socket&&);

		int receive(char *buffer, int buffer_size, Address &source);

		// returns: -1 for invalid packet, 0 for no packets in queue, >0: valid packet
		int receive(InPacket &packet, Address &source);

		void send(const char *data, int size, const Address &target);
		void send(const OutPacket &packet, const Address &target);

		void close();
		bool isValid() const { return m_fd != 0; }

	protected:
		int m_fd;
	};

	struct PacketInfo {
		PacketInfo() { }
		PacketInfo(SeqNumber packet_id, int current_id, int remote_id, int flags);

		i16 protocol_id;
		SeqNumber packet_id; //TODO: should we really save space here?
		i8 current_id, remote_id;
		i8 flags;

		enum {
			max_size = limits::packet_size,
			max_host_id = 127,
			header_size = sizeof(protocol_id) + sizeof(packet_id) + sizeof(current_id) + sizeof(remote_id) + sizeof(flags),
			valid_protocol_id = 0x1234,

			flag_first = 1,		// first packet for given frame, contains ack's
			flag_encrypted = 2,
			flag_compressed = 4,
			flag_lobby = 8,		// doesn't contain chunks, have to be handled differently
		};

		void save(Stream &sr) const;
		void load(Stream &sr);
	};

	class InPacket: public Stream {
	public:
		InPacket() :Stream(true) { }

		bool end() const { return m_pos == m_size; }
		const PacketInfo &info() const { return m_info; }
		SeqNumber packetId() const { return m_info.packet_id; }
		int currentId() const { return m_info.current_id; }
		int remoteId() const { return m_info.remote_id; }
		int flags() const { return m_info.flags; }

		int decodeInt() { return ::decodeInt(*this); }

		template <class T>
		void skip() {
			T temp;
			*this >> temp;
		}

	protected:
		virtual void v_load(void *ptr, int size) final;
		void ready(int new_size);
		friend class Socket;
		
		char m_data[limits::packet_size];
		PacketInfo m_info;
	};

	class OutPacket: public TempPacket {
	public:
		OutPacket() { }
		OutPacket(SeqNumber packet_id, int current_id, int remote_id, int flags);
	};

}


#endif
