/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "io.h"
#include "gfx/device.h"

#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/host.h"
#include "audio/device.h"

using namespace gfx;
using namespace game;
using namespace net;

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
		
	void connect(const char *server_name, int server_port) {
		if(m_mode != Mode::disconnected)
			disconnect();

		m_server_address = Address(resolveName(server_name), server_port);
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
						m_world->removeEntity(n);
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

			if(m_order) {
				TempPacket temp;
				temp << m_order;
				host->enqueChunk(temp, ChunkType::actor_order, 0);
				m_order.reset();
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

			if(m_order) {
				TempPacket temp;
				temp << m_order;
				host->enqueChunk(temp, ChunkType::actor_order, 0);
				m_order.reset();
			}
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

		int index = chunk.chunkId();

		if(index >= 0) {
			if(index < m_world->entityCount() && m_world->getEntity(index))
				m_world->removeEntity(index);

			if(chunk.type() == ChunkType::entity_full) {
				Entity *new_entity = Entity::construct(chunk);
				m_world->addEntity(PEntity(new_entity), index);
				if(new_entity->ref() == m_actor_ref) {
					Actor *actor = static_cast<Actor*>(new_entity);
					if(actor->currentOrder() == m_order_type && m_order_type != OrderTypeId::invalid) {
						printf("Order lag: %f\n", getTime() - m_order_send_time);
						m_order_type = OrderTypeId::invalid;
					}
				}
			}
		}
	}

	void replicateOrder(POrder &&order, EntityRef entity_ref) {
		if(entity_ref == m_actor_ref) {
			m_order = std::move(order);
			m_order_type = m_order->typeId();
			m_order_send_time = getTime();
		}
	}

private:
	EntityRef m_actor_ref;
	int m_server_id;
	Mode m_mode;
	PWorld m_world;

	POrder m_order;
	Address m_server_address;

	OrderTypeId::Type m_order_type;
	double m_order_send_time;
};

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
	
	if(!port || !server_port || !server_name) {
		printf("Port, server port or server name not specified!\n");
		return 0;
	}

	unique_ptr<Client> host(new Client(port));
	
	audio::initSoundMap();
	Config config = loadConfig("client");
	game::loadData();

	audio::initDevice();

	audio::setListenerPos(float3(0, 0, 0));
	audio::setListenerVelocity(float3(0, 0, 0));
	audio::setUnits(16.66666666);

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);
	setWindowPos(config.window_pos);
	pollEvents();

	printDeviceInfo();
	grabMouse(false);

	setBlendingMode(bmNormal);

	host->connect(server_name, server_port);
	
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

	IO io(config.resolution, world, host->actorRef(), config.profiler_enabled);

	double last_time = getTime();
	while(pollEvents() && !isKeyDown(Key_esc) && host->mode() == Client::Mode::connected) {
		double time = getTime();
		io.processInput();
		host->beginFrame();

		audio::tick();
		world->simulate((time - last_time) * config.time_multiplier);
		host->finishFrame();
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

