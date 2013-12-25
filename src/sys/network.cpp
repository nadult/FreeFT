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


	Address::Address() {
		memset(&m_data, 0, sizeof(m_data));
	}
	Address::Address(const char *name, int port) {
		ASSERT(port >= 0 && port < 65536);

		memset(&m_data, 0, sizeof(m_data));
		m_data.sin_family = AF_INET;
		m_data.sin_port = htons((unsigned short)port);

		if(name) {
			struct hostent *hp = gethostbyname(name);
			if(!hp)
				THROW("Error while getting host name");
			memcpy(&m_data.sin_addr, hp->h_addr_list[0], hp->h_length);
		}
		else {
			m_data.sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}

	bool Address::isValid() const {
		for(int c = 0; c < (int)sizeof(Address); c++)
			if(((char*)this)[c])
				return true;
		return false;
	}

	void Address::getIp(unsigned char elems[4]) const {
		unsigned int ip = ntohl(m_data.sin_addr.s_addr);
		for(int n = 0; n < 4; n++)
			elems[3 - n] = (ip >> (n * 8)) & 0xff;
	}

	int Address::getPort() const {
		return (int)ntohs(m_data.sin_port);
	}

	const string Address::toString() const {
		char buf[64];
		unsigned char elems[4];

		getIp(elems);
		sprintf(buf, "%d.%d.%d.%d:%d", (int)elems[0], (int)elems[1], (int)elems[2], (int)elems[3], getPort());

		return std::move(string(buf));
	}

	bool Address::operator==(const Address &addr) const {
		return memcmp(&m_data, &addr.m_data, sizeof(m_data)) == 0;
	}

	Socket::Socket(const Address &address) {
		m_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(m_fd < 0)
			THROW("Error while creating socket");

		if(bind(m_fd, (struct sockaddr*)&address.m_data, sizeof(address.m_data)) < 0) {
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
		socklen_t addr_len = sizeof(source.m_data);
		int len = recvfrom(m_fd, buffer, buffer_size, 0, (struct sockaddr*)&source.m_data, &addr_len);
		//TODO: handle errors
		return len < 0? 0 : len;
	}

	void Socket::send(const char *data, int size, const Address &target) {
		int ret = sendto(m_fd, data, size, 0, (struct sockaddr*)&target.m_data, sizeof(target.m_data));
		if(ret < 0) {
			//TODO: handle errors
		}
	}

	void InPacket::v_load(void *ptr, int count) {
		memcpy(ptr, m_data + m_pos, count);
		m_pos += count;
	}

	void InPacket::reset(int new_size) {
		m_size = new_size;
		m_pos = 0;
		m_exception_thrown = false;
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
		packet.reset(m_socket.receive(packet.m_data, sizeof(packet.m_data), address));
#ifdef LOG_PACKETS
		if(packet.size()) {
			printf("IN(%d) ", packet.size());
			fflush(stdout);
		}
#endif
		return packet.size() > 0;
	}

	void Host::send(const OutPacket &packet, const Address &address) {
		m_socket.send(packet.m_data, packet.size(), address);
#ifdef LOG_PACKETS
		if(packet.size()) {
			printf("OUT(%d) ", packet.size());
			fflush(stdout);
		}
#endif
	}

}
