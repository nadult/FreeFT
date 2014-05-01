/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>
#include "sys/platform.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/socket.h"
#include "net/lobby.h"
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
	LobbyServer(int port) :m_socket(lobbyServerAddress().port) { }

	struct ServerInfo: public ServerStatusChunk {
		ServerInfo(ServerStatusChunk chunk) :ServerStatusChunk(chunk), last_update_time(getTime()) { }
		double last_update_time;

		bool operator<(const ServerInfo &rhs) const {
			return address < rhs.address;
		}
	};

	void tick() {
		double time = getTime();

		while(true) {
			InPacket packet;
			Address source;

			int ret = m_socket.receive(packet, source);
			if(ret == 0)
				break;
			if(ret < 0 || !(packet.flags() & PacketInfo::flag_lobby))
				continue;
			
			try {
				LobbyChunkId::Type chunk_id;
				packet >> chunk_id;

				if(chunk_id == LobbyChunkId::server_status) {
					ServerStatusChunk chunk;
					packet >> chunk;
					chunk.address = source;
					
					auto it = m_servers.find(chunk.address);
					if(it == m_servers.end()) {
						it = m_servers.insert(make_pair(chunk.address, ServerInfo(chunk))).first;
						LOG("New server: %s (%s)\n", chunk.server_name.c_str(), source.toString().c_str());
					}
					else
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
						OutPacket out(0, -1, -1, PacketInfo::flag_lobby);
						out << LobbyChunkId::join_request;
						out.pack(source.ip, source.port);
						m_socket.send(out, server_address);
					}
				}
				else if(chunk_id == LobbyChunkId::server_list_request) {
					OutPacket out(0, -1, -1, PacketInfo::flag_lobby);
					out << LobbyChunkId::server_list;
					LOG("Client wants info: %s\n", source.toString().c_str());

					for(auto it = m_servers.begin(); it != m_servers.end(); ++it) {
						TempPacket temp;
						it->second.save(temp);
						if(temp.pos() <= out.spaceLeft())
							out.saveData(temp.data(), temp.pos());
					}

					m_socket.send(out, source);
				}
			}
			catch(...) {
				continue;
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

int safe_main(int argc, char **argv)
{
	printf("FreeFT::lobby_server; built " __DATE__ " " __TIME__ "\nPress Ctrl+C to quit\n");
	LobbyServer server(12345);

	while(true) {
		server.tick();
		sleep(0.01);
	}

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

