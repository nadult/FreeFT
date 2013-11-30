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

	protected:
		friend class Socket;
		struct sockaddr_in data;
	};

	class Socket {
	public:
		Socket(const Address &address);
		~Socket();
		
		void operator=(const Socket&) = delete;
		Socket(const Socket&) = delete;

		int receive(char *buffer, int buffer_size, Address &source);
		void send(const char *data, int size, const Address &target);
		

	protected:
		int fd;
	};


}

#endif
