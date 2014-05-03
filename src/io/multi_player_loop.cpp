/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/multi_player_loop.h"
#include "gfx/device.h"
#include "net/client.h"
#include "sys/config.h"

using namespace net;

namespace io {

	static void parseServerList(std::map<Address, net::ServerStatusChunk> &servers, InPacket &packet) {
		while(packet.pos() < packet.size()) {
			ServerStatusChunk chunk;
			packet >> chunk;

			if(chunk.address.isValid())
				servers[chunk.address] = chunk;
		}
	}

	static Address findServer(int local_port) {
		using namespace net;

		Address local_addr((u16)local_port);
		Socket socket(local_addr); //TODO: use socket from Client class
		Address lobby_address = lobbyServerAddress();

		OutPacket request(0, -1, -1, PacketInfo::flag_lobby);
		request << LobbyChunkId::server_list_request;
		socket.send(request, lobby_address);

		double start_time = getTime();

		while(getTime() - start_time < 5.0) {
			InPacket packet;
			Address source;
			int ret = socket.receive(packet, source);
			if(ret <= 0) {
				sleep(0.01);
				continue;
			}

			try {
				LobbyChunkId::Type chunk_id;
				packet >> chunk_id;

				if(chunk_id == LobbyChunkId::server_list) {
					std::map<Address, ServerStatusChunk> servers;
					parseServerList(servers, packet);

					if(servers.empty()) {
						printf("No servers currently active\n");
						return Address();
					}
					else {
						auto it = servers.begin();
						//TODO: punch through
						return it->second.address;
					}
				}
			}
			catch(...) {
				continue;
			}


		}
				
		printf("Timeout when connecting to lobby server\n");
		return Address();
	}

	MultiPlayerLoop::MultiPlayerLoop(const string &server_name, int port) {
		Config config = loadConfig("client");

		Address server_address = findServer(port);
		//Address server_address = server_name.empty()? findServer(port) : Address(resolveName(server_name.c_str()), server_port);
		if(!server_address.isValid())
			THROW("Invalid server address\n");

		m_client.reset(new Client(port));
		m_client->connect(server_address);
		
		while(m_client->mode() != Client::Mode::connected) {
			m_client->beginFrame();
			m_client->finishFrame();
			sleep(0.05);
		}

		m_world = m_client->world();
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, m_client->actorRef(), config.profiler_enabled));
	}

	bool MultiPlayerLoop::tick(double time_diff) {
		using namespace gfx;

		m_controller->update();
		m_client->beginFrame();

		m_world->simulate(time_diff);
		m_client->finishFrame();
		m_controller->updateView(time_diff);

		m_controller->draw();
		return !isKeyPressed(Key_esc);
	}

}
