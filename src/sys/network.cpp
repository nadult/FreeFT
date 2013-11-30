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


namespace net {


	Address::Address() {
		memset(&data, 0, sizeof(data));
	}
	Address::Address(const char *name, int port) {
		ASSERT(port >= 0 && port < 65536);

		memset(&data, 0, sizeof(data));
		data.sin_family = AF_INET;
		data.sin_port = htons((unsigned short)port);

		if(name) {
			struct hostent *hp = gethostbyname(name);
			if(!hp)
				THROW("Error while getting host name");
			memcpy(&data.sin_addr, hp->h_addr_list[0], hp->h_length);
		}
		else {
			data.sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}

	Socket::Socket(const Address &address) {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(fd < 0)
			THROW("Error while creating socket");

		if(bind(fd, (struct sockaddr*)&address.data, sizeof(address.data)) < 0) {
			close(fd);
			THROW("Error while binding address to socket");
		}

#ifdef _WIN32
		THROW("use ioctlsocket");
#else
		fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
	}
	Socket::~Socket() {
		close(fd);
	}
	
	int Socket::receive(char *buffer, int buffer_size, Address &source) {
		socklen_t addr_len = sizeof(source.data);
		int len = recvfrom(fd, buffer, buffer_size, 0, (struct sockaddr*)&source.data, &addr_len);
		//TODO: handle errors
		return len < 0? 0 : len;
	}

	void Socket::send(const char *data, int size, const Address &target) {
		int ret = sendto(fd, data, size, 0, (struct sockaddr*)&target.data, sizeof(target.data));
		if(ret < 0) {
			//TODO: handle errors
		}
	}

}
