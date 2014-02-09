/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_NETWORK_H
#define SYS_NETWORK_H
#include "base.h"


namespace net {

	u32 resolveName(const char *name);
	void decomposeIp(u32 ip, u8 elems[4]);

	void encodeInt3(Stream&, const int3 &value);
	const int3 decodeInt3(Stream&);

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
		friend class LocalHost;
		friend class RemoteHost;
		
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

		friend class RemoteHost;
		friend class LocalHost;
		char m_data[PacketInfo::max_size];
	};

	class OutPacket: public TempPacket {
	public:
		OutPacket() { }
		OutPacket(SeqNumber packet_id, int current_id, int remote_id, int flags);
	};

	struct JoinAcceptPacket {
		void save(Stream&) const;
		void load(Stream&);

		string map_name;
		i32 actor_id;
	};

	//TODO: change name
	enum class ChunkType: char {
		// These are reserved for internal use in RemoteHost, it's illegal to use them
		invalid,
		multiple_chunks,
		ack,

		join,
		join_accept,
		join_refuse,
		join_complete,
		leave,

		timestamp,
		entity_full,
		entity_delete,
		entity_update,

		actor_order,
	};

	enum class RefuseReason: char {
		too_many_clients,
	};

}

SERIALIZE_AS_POD(net::SeqNumber)

#endif
