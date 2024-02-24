// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include <memory.h>
#include <cstdio>
#include "sys/config.h"
#include "net/socket.h"
#include "net/base.h"
#include <list>
#include <algorithm>
#include <map>

using namespace net;

#define LOGGING

#ifdef LOGGING
	#define LOG(...)	printf(__VA_ARGS__)
#else
	#define LOG(...)
#endif


class LobbyServer {
public:
	// TODO: socket only on specific interface?
	LobbyServer(int port) {
		m_socket = move(Socket::make(port).get());
	}

	struct ServerInfo: public ServerStatusChunk {
		ServerInfo(ServerStatusChunk chunk) :ServerStatusChunk(chunk), last_update_time(getTime()) { }
		double last_update_time;

		bool operator<(const ServerInfo &rhs) const {
			return address < rhs.address;
		}
	};

	void tick() {
		double time = getTime();
		InPacket packet;

		while(true) {
			Address source;

			auto result = m_socket.receive(packet, source);
			if(result == RecvResult::empty)
				break;
			if(result == RecvResult::invalid || !(packet.info.flags & PacketFlag::lobby))
				continue;

			{ // TODO: proper error handling
				LobbyChunkId chunk_id;
				packet >> chunk_id;

				if(chunk_id == LobbyChunkId::server_status) {
					ServerStatusChunk chunk;
					chunk.load(packet);
					chunk.address = source;
					
					auto it = m_servers.find(chunk.address);
					if(it == m_servers.end()) {
						it = m_servers.emplace(chunk.address, ServerInfo(chunk)).first;
						LOG("New server: %s (%s)\n", chunk.server_name.c_str(),
							source.toString().c_str());
					} else
						it->second = chunk;
				}
				else if(chunk_id == LobbyChunkId::server_down) {
					auto it = m_servers.find(source);
					if(it != m_servers.end()) {
						LOG("Server going down: %s (%s)\n", it->second.server_name.c_str(), it->second.address.toString().c_str());
						m_servers.erase(it);
					}
				}
				else if(chunk_id == LobbyChunkId::join_request) {
					Address server_address;
					packet.unpack(server_address.ip, server_address.port);
					auto it = m_servers.find(server_address);

					if(it != m_servers.end()) {
						LOG("Punching through for: %s to: %s\n", source.toString().c_str(),
							server_address.toString().c_str());

						OutPacket out({0, -1, -1, PacketFlag::lobby});
						out << LobbyChunkId::join_request;
						out.pack(source.ip, source.port);
						m_socket.send(out.data(), server_address);
					}
				} else if(chunk_id == LobbyChunkId::server_list_request) {
					// TODO: These interfaces could be improved...
					OutPacket out({0, -1, -1, PacketFlag::lobby});
					out << LobbyChunkId::server_list;
					LOG("Client wants info: %s\n", source.toString().c_str());

					for(auto &[addr, sinfo] : m_servers) {
						char buffer[limits::packet_size];
						auto temp = memorySaver(buffer);
						sinfo.save(temp);
						if(temp.size() <= out.capacityLeft())
							out.saveData(temp.data());
					}

					m_socket.send(out.data(), source);
				}
			}
		}

		auto it = m_servers.begin();
		while(it != m_servers.end()) {
			if(it->second.last_update_time + 20.0 < time) {
				LOG("Server timeout: %s (%s)\n", it->second.server_name.c_str(), it->second.address.toString().c_str());
				it = m_servers.erase(it);
			}
			else
				it++;
		}
	}

private:
	Socket m_socket;
	std::map<Address, ServerInfo> m_servers;
};

static bool s_is_closing = false;

void onCtrlC() {
	s_is_closing = true;
}

int main(int argc, char **argv) {
	int port = lobbyServerAddress()->port;

	if(argc >= 3 && strcmp(argv[1], "-p") == 0) {
		int tport = atoi(argv[2]);
		if(tport > 0 && tport < 65536)
			port = tport;
	}

	printf("FreeFT::lobby_server; built " __DATE__ " " __TIME__ "\nPress Ctrl+C to quit\n");
	handleCtrlC(onCtrlC);
	LobbyServer server(port);
	printf("Lobby server port: %d\n", port);

	while(!s_is_closing) {
		server.tick();
		sleep(0.01);
	}

	printf("Closing...\n");
	return 0;
}
