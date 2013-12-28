/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/network.h"
#include <cstdlib>
#include <unistd.h>

#ifdef _WIN32

typedef int socklen_t;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#endif

//#define LOG_PACKETS

namespace net {



	static void toSockAddr(const Address &in, sockaddr_in *out) {
		memset(out, 0, sizeof(sockaddr_in));
		out->sin_addr.s_addr = htonl(in.ip);
		out->sin_port = htons(in.port);
	}

	static void fromSockAddr(const sockaddr_in *in, Address &out) {
		out.ip = ntohl(in->sin_addr.s_addr);
		out.port = ntohs(in->sin_port);
	}

	u32 resolveName(const char *name) {
		DASSERT(name);

		struct hostent *hp = gethostbyname(name);
		if(!hp)
			THROW("Error while getting host name");
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
		return htonl(addr.sin_addr.s_addr);
	}
	
	void decomposeIp(u32 ip, u8 elems[4]) {
		for(int n = 0; n < 4; n++)
			elems[3 - n] = (ip >> (n * 8)) & 0xff;
	}

	Address::Address(u16 port) :port(port), ip(htonl(INADDR_ANY)) { }

	const string Address::toString() const {
		char buf[64];
		unsigned char elems[4];

		decomposeIp(ip, elems);
		sprintf(buf, "%d.%d.%d.%d:%d", (int)elems[0], (int)elems[1], (int)elems[2], (int)elems[3], (int)port);
		return std::move(string(buf));
	}

	Socket::Socket(const Address &address) {
		m_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(m_fd < 0)
			THROW("Error while creating socket");

		sockaddr_in addr;
		toSockAddr(address, &addr);
		if(bind(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			::close(m_fd);
			THROW("Error while binding address to socket");
		}

#ifdef _WIN32
		THROW("use ioctlsocket");
#else
		fcntl(m_fd, F_SETFL, O_NONBLOCK);
#endif
	}

	Socket::~Socket() {
		close();
	}

	void Socket::close() {
		if(m_fd) {
			::close(m_fd);
			m_fd = 0;
		}
	}

	Socket::Socket(Socket &&rhs) :m_fd(rhs.m_fd) {
		rhs.m_fd = 0;
	}

	void Socket::operator=(Socket &&rhs) {
		swap(m_fd, rhs.m_fd);
		rhs.close();
	}
	
	int Socket::receive(char *buffer, int buffer_size, Address &source) {
		sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int len = recvfrom(m_fd, buffer, buffer_size, 0, (struct sockaddr*)&addr, &addr_len);
		fromSockAddr(&addr, source);
		//TODO: handle errors
		return len < 0? 0 : len;
	}

	void Socket::send(const char *data, int size, const Address &target) {
		sockaddr_in addr;
		toSockAddr(target, &addr);
		int ret = sendto(m_fd, data, size, 0, (struct sockaddr*)&addr, sizeof(addr));
		if(ret < 0) {
			//TODO: handle errors
		}
	}
		
	PacketInfo::PacketInfo(int packet_id, int time_stamp, int client_id_, int flags_)
		:protocol_id(valid_protocol_id), packet_id(packet_id), time_stamp(time_stamp) {
		DASSERT(client_id_ >= -1 && client_id <= 127);
		DASSERT((flags_ & ~0xff) == 0);

		client_id = client_id_;
		flags = flags_;
	}

	
	void PacketInfo::save(Stream &sr) const {
		sr.pack(protocol_id, packet_id, time_stamp, client_id, flags);
	}
	void PacketInfo::load(Stream &sr) {
		sr.unpack(protocol_id, packet_id, time_stamp, client_id, flags);
	}

	void InPacket::v_load(void *ptr, int count) {
		memcpy(ptr, m_data + m_pos, count);
		m_pos += count;
	}

	void InPacket::ready(int new_size) {
		m_size = new_size;
		m_pos = 0;
		m_exception_thrown = false;
		*this >> m_info;
	}
		
	OutPacket::OutPacket(int packet_id, int time_stamp, int client_id, int flags) :OutPacket() {
		*this << PacketInfo(packet_id, time_stamp, client_id, flags);
	}

	void OutPacket::v_save(const void *ptr, int count) {
		if(m_pos + count > (int)sizeof(m_data))
			THROW("not enough space in buffer (%d space left, %d needed)", spaceLeft(), (int)count);

		memcpy(m_data + m_pos, ptr, count);
		m_pos += count;
		if(m_pos > m_size)
			m_size = m_pos;
	}

	bool Host::receive(InPacket &packet, Address &address) {
		while(true) {
			int new_size = m_socket.receive(packet.m_data, sizeof(packet.m_data), address);
			if(new_size == 0)
				return false;

			if(new_size >= PacketInfo::min_size) {
				packet.ready(new_size);
				if(packet.info().protocol_id == PacketInfo::valid_protocol_id)
					return true;
			}
		}
	}

	void Host::send(const OutPacket &packet, const Address &address) {
		m_socket.send(packet.m_data, packet.size(), address);
	}

}
