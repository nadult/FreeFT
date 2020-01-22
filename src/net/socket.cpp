// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifdef _WIN32

typedef int socklen_t;
#include <winsock2.h>

#else

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif

#include <unistd.h>
#include "net/socket.h"

//#define RELIABILITY_TEST
#ifdef RELIABILITY_TEST
static bool isDropped() {
	return rand() % 1024 < 500;
}
#endif

//#define LOG_PACKETS

namespace net {

	static void toSockAddr(const Address &in, sockaddr_in *out) {
		memset(out, 0, sizeof(sockaddr_in));
#ifdef _WIN32
		out->sin_family = AF_INET;
#endif
		out->sin_addr.s_addr = htonl(in.ip);
		out->sin_port = htons(in.port);
	}

	static void fromSockAddr(const sockaddr_in *in, Address &out) {
		out.ip = ntohl(in->sin_addr.s_addr);
		out.port = ntohs(in->sin_port);
	}

	Ex<u32> resolveName(ZStr name) {
		DASSERT(name);

		struct hostent *hp = gethostbyname(name.c_str());
		if(!hp)
			return FWK_ERROR("Error while getting host name");
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
		return htonl(addr.sin_addr.s_addr);
	}
	
	void decomposeIp(u32 ip, u8 elems[4]) {
		for(int n = 0; n < 4; n++)
			elems[3 - n] = (ip >> (n * 8)) & 0xff;
	}

	int randomPort() {
		return 50000 + rand() % 16530;
	}

	Ex<Address> lobbyServerAddress() {
		return Address(EX_PASS(resolveName("localhost")), 50000);
		return Address(EX_PASS(resolveName("89.74.58.32")), 50000);
	}

	Address::Address(u16 port) :port(port), ip(htonl(INADDR_ANY)) { }

	const string Address::toString() const {
		char buf[64];
		unsigned char elems[4];

		decomposeIp(ip, elems);
		sprintf(buf, "%d.%d.%d.%d:%d", (int)elems[0], (int)elems[1], (int)elems[2], (int)elems[3], (int)port);
		return string(buf);
	}

	PacketInfo::PacketInfo(SeqNumber packet_id, int current_id_, int remote_id_, PacketFlags flags_)
		: protocol_id(valid_protocol_id), packet_id(packet_id) {
		DASSERT(current_id_ >= -1 && current_id <= max_host_id);
		DASSERT(remote_id_ >= -1 && remote_id <= max_host_id);

		current_id = current_id_;
		remote_id = remote_id_;
		flags = flags_;
	}

	void PacketInfo::save(MemoryStream &sr) const {
		sr.pack(protocol_id, packet_id, current_id, remote_id, u8(flags));
	}
	void PacketInfo::load(MemoryStream &sr) {
		u8 flags_;
		sr.unpack(protocol_id, packet_id, current_id, remote_id, flags_);
		flags = PacketFlags(flags_);
		// TODO: checks
	}

	InPacket::InPacket() : MemoryStream(cspan("", 0)) {}
	InPacket::InPacket(PodVector<char> data) : MemoryStream(move(data), true) { *this >> info; }
	InPacket::InPacket(InPacket &&) = default;
	InPacket &InPacket::operator=(InPacket &&rhs) = default;

	OutPacket::OutPacket(PacketInfo info)
		: MemoryStream(memorySaver(limits::packet_size)), info(info) {
		*this << info;
	}

	Ex<Socket> Socket::make(const Address &address) {
#ifdef _WIN32
		static bool wsock_initialized = false;
		if(!wsock_initialized) {
			WSAData data;
			WSAStartup(0x2020, &data);
			wsock_initialized = true;
		}
#endif
		auto fd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
		if(fd == INVALID_SOCKET)
#else
		if(fd == -1)
#endif
			return FWK_ERROR("Error while creating socket");

		sockaddr_in addr;
		toSockAddr(address, &addr);
		if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			::close(fd);
			return FWK_ERROR("Error while binding address to socket");
		}

#ifdef _WIN32
		unsigned long int non_blocking = 1;
		ioctlsocket(fd, FIONBIO, &non_blocking);
#else
		fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
		Socket out;
		out.m_fd = fd;
		return out;
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
		if(&rhs == this)
			return;
		swap(m_fd, rhs.m_fd);
		rhs.close();
	}

	int Socket::receive(Span<char> buffer, Address &source) {
		DASSERT(m_fd);

		sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int len =
			recvfrom(m_fd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&addr, &addr_len);
		fromSockAddr(&addr, source);

#ifdef RELIABILITY_TEST
		if(isDropped())
			return 0;
#endif

		//TODO: handle errors
		return len < 0? 0 : len;
	}

	RecvResult Socket::receive(InPacket &packet, Address &source) {
		auto data = packet.extractBuffer();
		data.resize(limits::recv_packet_size);

		auto new_size = receive(data, source);
		if(new_size == 0)
			return RecvResult::empty;
		if(new_size < PacketInfo::header_size)
			return RecvResult::invalid;

		data.resize(new_size);
		packet = move(data);
		return packet.info.valid()? RecvResult::valid : RecvResult::invalid;
	}

	void Socket::send(CSpan<char> data, const Address &target) {
		DASSERT(m_fd);

		sockaddr_in addr;
		toSockAddr(target, &addr);
		int ret = sendto(m_fd, data.data(), data.size(), 0, (struct sockaddr *)&addr, sizeof(addr));
		if(ret < 0) {
			//TODO: handle errors
		}
	}
}
