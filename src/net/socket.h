/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_SOCKET_H
#define NET_SOCKET_H
#include "base.h"


namespace net {

	u32 resolveName(const char *name);
	void decomposeIp(u32 ip, u8 elems[4]);

	void encodeInt3(Stream&, const int3 &value);
	const int3 decodeInt3(Stream&);

	int randomPort();

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
		bool operator==(const SeqNumber &rhs) { return value == rhs.value; }
		bool operator!=(const SeqNumber &rhs) { return value != rhs.value; }

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

	class InPacket;
	class OutPacket;

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
			max_size = 1400,
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

		template <class T>
		void skip() {
			T temp;
			*this >> temp;
		}

	protected:
		virtual void v_load(void *ptr, int size) final;
		void ready(int new_size);
		friend class Socket;
		
		char m_data[PacketInfo::max_size];
		PacketInfo m_info;
	};

	class TempPacket: public Stream {
	public:
		TempPacket() :Stream(false) { m_size = 0; }

		const char *data() const { return m_data; }
		int spaceLeft() const { return sizeof(m_data) - m_pos; }

	protected:
		virtual void v_save(const void *ptr, int size) final;

		friend class Socket;
		char m_data[PacketInfo::max_size];
	};

	class OutPacket: public TempPacket {
	public:
		OutPacket() { }
		OutPacket(SeqNumber packet_id, int current_id, int remote_id, int flags);
	};

}

SERIALIZE_AS_POD(net::SeqNumber)

#endif
