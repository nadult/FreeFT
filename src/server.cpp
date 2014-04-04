/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "gfx/device.h"
#include "io/io.h"

#include "navi_map.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/host.h"
#include <list>
#include <algorithm>

using namespace gfx;
using namespace game;
using namespace net;
using namespace io;

class Server: public net::LocalHost, game::Replicator {
public:
	Server(int port) :LocalHost(Address(port)), m_timestamp(0), m_client_count(0) { }

	enum {
		max_clients = 32,
		client_timeout = 10,
	};

	enum class ClientMode {
		invalid,
		connecting,
		connected,
		to_be_removed,
	};

	struct Client {
		Client() :mode(ClientMode::invalid), host_id(-1) { }

		bool isValid() const { return host_id != -1 && mode != ClientMode::invalid; }

		ClientMode mode;
		BitVector update_map;
		EntityRef actor_ref;
		int host_id;
	};


	EntityRef spawnActor(const float3 &pos) {
		DASSERT(m_world);
		return m_world->addNewEntity<Actor>(pos, getProto("male", ProtoId::actor));
	}

	void disconnectClient(int client_id) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		m_clients[client_id].mode = ClientMode::to_be_removed;
	}

	void handleHostReceiving(RemoteHost &host, Client &client) {
		DASSERT(client.isValid());

		const Chunk *chunk = nullptr;

		while( const Chunk *chunk_ptr = host.getIChunk() ) {
			InChunk chunk(*chunk_ptr);

			if(chunk.type() == ChunkType::join) {
				client.actor_ref = spawnActor(float3(245 + frand() * 10.0f, 128, 335 + frand() * 10.0f));
//				printf("Client connected (cid:%d): %s\n", (int)r, host.address().toString().c_str());

				client.update_map.resize(m_world->entityCount() * 2);
				for(int n = 0; n < m_world->entityCount(); n++)
					if(m_world->refEntity(n))
						client.update_map[n] = true;

				TempPacket temp;
				temp << string(m_world->mapName()) << client.actor_ref;
				host.enqueChunk(temp, ChunkType::join_accept, 0);
			}
			if(chunk.type() == ChunkType::join_complete) {
				client.mode = ClientMode::connected;
				m_client_count++;
				host.verify(true);
				break;
			}
			else if(chunk.type() == ChunkType::leave) {	
				disconnectClient(host.currentId());
				break;
			}
			else if(client.mode == ClientMode::connected && chunk.type() == ChunkType::actor_order) {
				POrder order;
				chunk >> order;
				if(order)
					m_world->sendOrder(std::move(order), client.actor_ref);
				else
					printf("Invalid order!\n");
			}
		}
	}

	void handleHostSending(RemoteHost &host, Client &client) {
		DASSERT(client.isValid());
		beginSending(client.host_id);

		if(client.mode == ClientMode::connected) {
			for(int n = 0; n < (int)m_replication_list.size(); n++)
				client.update_map[m_replication_list[n]] = true;
			vector<int> &lost = host.lostUChunks();
			for(int n = 0; n < (int)lost.size(); n++)
				client.update_map[lost[n]] = true;
			lost.clear();

			//TODO: apply priorities to entities
			//TODO: priority for objects visible by client
				
			int idx = 0;
			BitVector &map = client.update_map;
			while(idx < m_world->entityCount()) {
				if(!map.any(idx >> BitVector::base_shift)) {
					idx = ((idx >> BitVector::base_shift) + 1) << BitVector::base_shift;
					continue;
				}

				if(map[idx]) {
					const Entity *entity = m_world->refEntity(idx);

					TempPacket temp;

					if(entity) {
						temp << entity->typeId() << *entity;
					}

					if(host.enqueUChunk(temp, entity? ChunkType::entity_full : ChunkType::entity_delete, idx, 1))
						map[idx] = false;
					else
						break;
				}

				idx++;
			}
		}
			
		finishSending();
	}

	void beginFrame() {
		InPacket packet;
		Address source;

		double time = getTime();
		m_current_time = time;

		LocalHost::receive();

		for(int h = 0; h < numRemoteHosts(); h++) {
			RemoteHost *host = getRemoteHost(h);

			if(!host)
				continue;
				
			if((int)m_clients.size() <= h)
				m_clients.resize(h + 1);
			Client &client = m_clients[h];
			if(!client.isValid()) {
				client.mode = ClientMode::connecting;
				client.host_id = h;
			}
			if(m_world->entityCount() > client.update_map.size())
				client.update_map.resize(m_world->entityCount() * 2);

			handleHostReceiving(*host, client);
		}
	}

	void finishFrame() {
		for(int h = 0; h < numRemoteHosts(); h++) {
			RemoteHost *host = getRemoteHost(h);

			if(!host)
				continue;
				
			if((int)m_clients.size() <= h)
				m_clients.resize(h + 1);
			Client &client = m_clients[h];
			if(!client.isValid()) {
				client.mode = ClientMode::connecting;
				client.host_id = h;
			}
			if(m_world->entityCount() > client.update_map.size())
				client.update_map.resize(m_world->entityCount() * 2);

			handleHostSending(*host, client);

			if(host->timeout() > 10.0) {
				printf("Disconnecting host (timeout)\n");
				disconnectClient(h);
			}
		}

		for(int h = 0; h < (int)m_clients.size(); h++) {
			Client &client = m_clients[h];
			if(client.mode == ClientMode::to_be_removed) {
				if(client.actor_ref) {
					m_world->removeEntity(client.actor_ref);
					client.actor_ref = EntityRef();
				}
				
				printf("Client disconnected: %s\n", getRemoteHost(h)->address().toString().c_str());
				removeRemoteHost(h);
				m_client_count--;
				client.mode = ClientMode::invalid;
			}
		}
		
		m_replication_list.clear();

		//TODO: check timeouts
		m_timestamp++;
	}

	void createWorld(const string &file_name) {
		//TODO: update clients
		m_world = new World(file_name, World::Mode::server, this);
	}

	PWorld world() { return m_world; }

	void replicateEntity(int entity_id) {
		m_replication_list.emplace_back(entity_id);
	}

private:
	vector<int> m_replication_list;
	vector<Client> m_clients;

	PWorld m_world;
	int m_timestamp;
	int m_client_count;
	double m_current_time;
};

int safe_main(int argc, char **argv)
{
	int port = 0;
	string map_name = "data/maps/mission05.mod";
	
	srand((int)getTime());

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "-p") == 0) {
			ASSERT(n + 1 < argc);
			port = atoi(argv[++n]);
		}
		else if(strcmp(argv[n], "-m") == 0) {
			ASSERT(n + 1 < argc);
			map_name = string("data/maps/") + argv[++n];
		}
	}
	if(!port) {
		printf("Port unspecified\n");
		return 0;
	}

	unique_ptr<Server> host(new Server(port));
	
	Config config = loadConfig("server");
	game::loadData();

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);
	setWindowPos(config.window_pos);
	pollEvents();

	printDeviceInfo();
	grabMouse(false);

	setBlendingMode(bmNormal);

	host->createWorld(map_name);
	PWorld world = host->world();

	for(int n = 0; n < world->entityCount(); n++) {
		Actor *actor = world->refEntity<Actor>(n);
		if(actor && actor->factionId() != 0)
			actor->attachAI<SimpleAI>();
	}

	WorldViewer viewer(world);
	IO io(config.resolution, world, viewer, EntityRef(), config.profiler_enabled);

	double last_time = getTime();
	while(pollEvents() && !isKeyDown(Key_esc)) {
		double time = getTime();

		io.update();
		host->beginFrame();

		double time_diff = (time - last_time) * config.time_multiplier;
		world->simulate(time_diff);
		host->finishFrame();
		viewer.update(time_diff);
		last_time = time;

		io.draw();

		swapBuffers();
		TextureCache::main_cache.nextFrame();
	}

	delete host.release();
	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		destroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

