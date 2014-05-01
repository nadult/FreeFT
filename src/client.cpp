/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "io/io.h"
#include "gfx/device.h"

#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/host.h"
#include "net/lobby.h"
#include "net/socket.h"
#include "audio/device.h"

using namespace gfx;
using namespace game;
using namespace net;
using namespace io;

class Client: public net::LocalHost, game::Replicator {
public:
	enum class Mode {
		disconnected,
		connecting,
		connected,
	};

	Client(int port)
		:LocalHost(Address(port)), m_mode(Mode::disconnected) {
			m_order_type = OrderTypeId::invalid;
		}
		
	void connect(Address address) {
		if(m_mode != Mode::disconnected)
			disconnect();

		m_server_address = address;
		m_server_id = addRemoteHost(m_server_address, -1);
		if(m_server_id != -1) {
			RemoteHost *host = getRemoteHost(m_server_id);
			host->enqueChunk("", 0, ChunkType::join, 0);
			m_mode = Mode::connecting;
		}
	}

	void disconnect() {
		if(m_mode == Mode::connected || m_mode == Mode::connecting) {
			RemoteHost *host = getRemoteHost(m_server_id);
			if(host) {
				beginSending(m_server_id);
				host->enqueUChunk("", 0, ChunkType::leave, 0, 0);
				finishSending();
				removeRemoteHost(m_server_id);
				m_server_id = -1;
			}

			m_mode = Mode::disconnected;
		}
	}

	~Client() {
	}
	
	Mode mode() const { return m_mode; }

	void beginFrame() {
		if(m_server_id == -1)
			return;
		RemoteHost *host = getRemoteHost(m_server_id);

		LocalHost::receive();

		if(m_mode == Mode::connecting) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::join_accept) {
					host->verify(true);
					string map_name;
					chunk >> map_name;
					chunk >> m_actor_ref;

					m_world = new World(map_name.c_str(), World::Mode::client, this);
					for(int n = 0; n < m_world->entityCount(); n++)
						m_world->removeEntity(m_world->toEntityRef(n));
					m_mode = Mode::connected;

					host->enqueChunk("", 0, ChunkType::join_complete, 0);
					printf("Joined to: %s (map: %s)\n", m_server_address.toString().c_str(), map_name.c_str());

				}
				else if(chunk.type() == ChunkType::join_refuse) {
					printf("Connection refused\n");
					exit(0);
				}
			}
		}
		else if(m_mode == Mode::connected) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::entity_delete || chunk.type() == ChunkType::entity_full)
					entityUpdate(chunk);
			}
		}
	}

	void finishFrame() {
		if(m_server_id == -1)
			return;
		RemoteHost *host = getRemoteHost(m_server_id);
		beginSending(m_server_id);

		if(m_mode == Mode::connected) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::entity_delete || chunk.type() == ChunkType::entity_full)
					entityUpdate(chunk);
			}

			for(int n = 0; n < (int)m_orders.size(); n++) {
				TempPacket temp;
				temp << m_orders[n];
				host->enqueChunk(temp, ChunkType::actor_order, 0);
			}
			m_orders.clear();
		}

		finishSending();
		m_timestamp++;

		if(host->timeout() > 10.0) {
			printf("Timeout\n");
			disconnect();
		}
	}

	EntityRef actorRef() const { return m_actor_ref; }

	PWorld world() { return m_world; }

protected:
	void entityUpdate(InChunk &chunk) {
		DASSERT(chunk.type() == ChunkType::entity_full || chunk.type() == ChunkType::entity_delete);

		m_world->removeEntity(m_world->toEntityRef(chunk.chunkId()));

		if(chunk.type() == ChunkType::entity_full) {
			Entity *new_entity = Entity::construct(chunk);
			m_world->addEntity(PEntity(new_entity), chunk.chunkId());

			if(new_entity->ref() == m_actor_ref) {
				Actor *actor = static_cast<Actor*>(new_entity);
				if(actor->currentOrder() == m_order_type && m_order_type != OrderTypeId::invalid) {
					printf("Order lag: %f\n", getTime() - m_order_send_time);
					m_order_type = OrderTypeId::invalid;
				}
			}
		}
	}

	void replicateOrder(POrder &&order, EntityRef entity_ref) {
		//TODO: handle cancel_prev
		if(entity_ref == m_actor_ref) {
			m_order_type = order->typeId();
			m_orders.emplace_back(std::move(order));
			m_order_send_time = getTime();
		}
	}

private:
	EntityRef m_actor_ref;
	int m_server_id;
	Mode m_mode;
	PWorld m_world;

	Address m_server_address;

	vector<POrder> m_orders;
	bool m_order_cancels_prev;
	OrderTypeId::Type m_order_type;
	double m_order_send_time;
};

void parseServerList(std::map<Address, net::ServerStatusChunk> &servers, InPacket &packet) {
	while(packet.pos() < packet.size()) {
		ServerStatusChunk chunk;
		packet >> chunk;

		if(chunk.address.isValid())
			servers[chunk.address] = chunk;
	}
}

Address findServer(int local_port) {
	using namespace net;

	Address local_addr((u16)local_port);
	Socket socket(local_addr);
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

int safe_main(int argc, char **argv)
{
	int port = 0, server_port = 0;;
	const char *server_name = NULL;

	srand((int)getTime());

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "-p") == 0) {
			ASSERT(n + 1 < argc);
			port = atoi(argv[++n]);
		}
		else if(strcmp(argv[n], "-s") == 0) {
			ASSERT(n + 2 < argc);
			server_name = argv[++n];
			server_port = atoi(argv[++n]);
		}
	}
	
	if(!port) {
		printf("Port not specified!\n");
		return 0;
	}

	Address server_address = server_name? Address(resolveName(server_name), server_port) : findServer(port);
	if(!server_address.isValid()) {
		printf("Invalid server address\n");
		return 0;
	}

	unique_ptr<Client> host(new Client(port));
	
	audio::initSoundMap();
	Config config = loadConfig("client");
	game::loadData();
	audio::initDevice();

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);
	setWindowPos(config.window_pos);
	pollEvents();

	printDeviceInfo();
	grabMouse(false);

	setBlendingMode(bmNormal);

	host->connect(server_address);
	
	while(host->mode() != Client::Mode::connected) {
		host->beginFrame();
		host->finishFrame();
		sleep(0.05);
	}

	PWorld world = host->world();

	while(!world->refEntity(host->actorRef())) {
		host->beginFrame();
		host->finishFrame();
		sleep(0.01);
	}

	WorldViewer viewer(world, host->actorRef());
	IO io(config.resolution, world, viewer, host->actorRef(), config.profiler_enabled);

	double last_time = getTime();
	while(pollEvents() && !isKeyDown(Key_esc) && host->mode() == Client::Mode::connected) {
		double time = getTime();

		io.update();
		host->beginFrame();

		audio::tick();
		double time_diff = (time - last_time) * config.time_multiplier;
		world->simulate(time_diff);
		host->finishFrame();
		viewer.update(time_diff);
		last_time = time;

		io.draw();

		swapBuffers();
		TextureCache::main_cache.nextFrame();
	}

	host->disconnect();

	delete host.release();

	audio::freeDevice();
	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		audio::freeDevice();
		destroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

