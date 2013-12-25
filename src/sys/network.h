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

	class Address {
	public:
		Address();
		Address(const char *name, int port);

		bool isValid() const;
		void getIp(unsigned char elems[4]) const;
		int getPort() const;
		const string toString() const;

		bool operator==(const Address&) const;

	protected:
		friend class Socket;
		struct sockaddr_in m_data; //TODO: store it in different way (portable)
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
		PacketInfo(int packet_id, int time_stamp, int client_id_, int flags_) :packet_id(packet_id), time_stamp(time_stamp) {
			DASSERT(client_id_ >= 0 && client_id < 256);
			DASSERT((flags_ & ~0xff) == 0);

			client_id = client_id_;
			flags = flags_;
		}

		enum { max_size = 1400 };

		i32 packet_id;
		i32 time_stamp;
		i8 client_id;
		i8 flags;

		enum {
			flag_need_ack = 1,
		};

		void save(Stream &sr) const { sr.pack(packet_id, time_stamp, client_id, flags); }
		void load(Stream &sr) { sr.unpack(packet_id, time_stamp, client_id, flags); }
	};

	class InPacket: public Stream {
	public:
		InPacket() :Stream(true) { }

		bool end() const { return m_pos == m_size; }

	protected:
		virtual void v_load(void *ptr, int size);
		void reset(int new_size);
		friend class Host;
		
		char m_data[PacketInfo::max_size];
	};

	class OutPacket: public Stream {
	public:
		OutPacket() :Stream(false) { m_size = 0; }

		int spaceLeft() const { return sizeof(m_data) - m_pos; }

	protected:
		virtual void v_save(const void *ptr, int size);

		friend class Host;
		char m_data[PacketInfo::max_size];
	};
	
	class Host {
	public:
		Host(const net::Address &address) :m_socket(address) { }

		bool receive(InPacket &packet, Address &source);
		void send(const OutPacket &packet, const Address &target);

		virtual ~Host() { }
		virtual void action() = 0;
		virtual bool isConnected() const = 0;


	protected:
		net::Socket m_socket;
	};


}

#endif
