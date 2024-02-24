// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "net/base.h"

namespace net {
Ex<u32> resolveName(ZStr);
void decomposeIp(u32 ip, u8 elems[4]);
int randomPort();
Ex<Address> lobbyServerAddress();

// First: first packet for given frame, contains ack's
// Lobby: doesn't contain chunks, have to be handled differently
DEFINE_ENUM(PacketFlag, first, encrypted, compressed, lobby);
using PacketFlags = EnumFlags<PacketFlag>;

struct PacketInfo {
	PacketInfo() {}
	PacketInfo(SeqNumber packet_id, int current_id, int remote_id, PacketFlags);

	bool valid() const { return protocol_id == valid_protocol_id; }

	i16 protocol_id;
	SeqNumber packet_id; //TODO: should we really save space here?
	i8 current_id, remote_id;
	PacketFlags flags;

	static constexpr int max_size = limits::packet_size, max_host_id = 127,
						 header_size = sizeof(protocol_id) + sizeof(packet_id) +
									   sizeof(current_id) + sizeof(remote_id) + sizeof(flags),
						 valid_protocol_id = 0x1234;

	void save(MemoryStream &sr) const;
	void load(MemoryStream &sr);
};

DEFINE_ENUM(RecvResult, empty, invalid, valid);

struct InPacket : public MemoryStream {
	InPacket();
	InPacket(PodVector<char>);
	InPacket(InPacket &&);
	InPacket &operator=(InPacket &&);

	int currentId() const { return info.current_id; }
	int packetId() const { return info.packet_id; }
	PacketFlags flags() const { return info.flags; }
	int decodeInt() { return ::decodeInt(*this); }

	PacketInfo info;
};

struct OutPacket : public MemoryStream {
	OutPacket(PacketInfo);
	PacketInfo info;
};

class Socket {
  public:
	static Ex<Socket> make(const Address &address);
	Socket() : m_fd(0) {}
	~Socket();

	void operator=(const Socket &) = delete;
	Socket(const Socket &) = delete;

	void operator=(Socket &&);
	Socket(Socket &&);

	int receive(Span<char> buffer, Address &source);
	// Returns true if packet received; packet may be invalid
	RecvResult receive(InPacket &, Address &source);

	void send(CSpan<char>, const Address &);
	//void send(const OutPacket &, const Address &);

	void close();
	bool isValid() const { return m_fd != 0; }

  protected:
	int m_fd;
};

}
