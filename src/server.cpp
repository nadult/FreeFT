/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"

#include "navi_map.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/container.h"
#include "game/door.h"
#include "game/item.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/host.h"
#include <list>
#include <algorithm>

using namespace gfx;
using namespace game;
using namespace net;


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
		PEntity actor(new Actor(getProto("male", ProtoId::actor)));
		actor->setPos(pos);
		int id = m_world->addEntity(std::move(actor));
		return m_world->getEntity(id)->ref();
	}

	void disconnectClient(int client_id) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		m_clients[client_id].mode = ClientMode::to_be_removed;
	}

	void handleHost(RemoteHost &host, Client &client) {
		DASSERT(client.isValid());
		beginSending(client.host_id);

		const Chunk *chunk = nullptr;

		while( const Chunk *chunk_ptr = host.getIChunk() ) {
			InChunk chunk(*chunk_ptr);

			if(chunk.type() == ChunkType::join) {
				client.actor_ref = spawnActor(float3(245 + frand() * 10.0f, 128, 335 + frand() * 10.0f));
//				printf("Client connected (cid:%d): %s\n", (int)r, host.address().toString().c_str());

				client.update_map.resize(m_world->entityCount() * 2);
				for(int n = 0; n < m_world->entityCount(); n++)
					if(m_world->getEntity(n))
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
					const Entity *entity = m_world->getEntity(idx);

					TempPacket temp;

					if(entity) {
						temp << entity->entityType() << *entity;
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

	void action() {
		InPacket packet;
		Address source;

		double time = getTime();
		m_current_time = time;

		LocalHost::beginFrame();

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

			handleHost(*host, client);
		}
		
		LocalHost::finishFrame();

		for(int h = 0; h < (int)m_clients.size(); h++) {
			Client &client = m_clients[h];
			if(client.mode == ClientMode::to_be_removed) {
				if(client.actor_ref) {
					m_world->removeEntity(m_world->refEntity(client.actor_ref));
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

	int2 view_pos(-1000, 500);

	PFont font = Font::mgr["liberation_32"];

	host->createWorld(map_name);
	PWorld world = host->world();

	double last_time = getTime();
	vector<int3> path;
	int3 last_pos(0, 0, 0);
	float3 target_pos(0, 0, 0);

	int inventory_sel = -1, container_sel = -1;
	string prof_stats;
	double stat_update_time = getTime();

	while(pollEvents()) {
		double loop_start = profiler::getTime();
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world->pixelIntersect(getMousePos() + view_pos, collider_all|visibility_flag);
		if(isect.isEmpty() || isect.isTile())
			isect = world->trace(ray, nullptr, collider_all|visibility_flag);

		double time = getTime();

		world->updateNaviMap(false);
		world->simulate((time - last_time) * config.time_multiplier);
		last_time = time;

		static int counter = 0;
		if(host)// && counter % 30 == 0)
			host->action();
		counter++;

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);
		
		if(!isect.isEmpty()) {		
			FBox box = world->refBBox(isect);
			renderer.addBox(box, Color(255, 255, 255, 100));
		}

	//	world->updateVisibility(actor->boundingBox());
		world->addToRender(renderer);

		renderer.render();
		lookAt(view_pos);
			
		lookAt({0, 0});
		drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
		drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

		DTexture::bind0();
		drawQuad(0, 0, 250, config.profiler_enabled? 300 : 50, Color(0, 0, 0, 80));
		
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
		float3 isect_pos = ray.at(isect.distance());
		font->drawShadowed(int2(0, 0), Color::white, Color::black,
				"View:(%d %d)\nRay:(%.2f %.2f %.2f)\n",
				view_pos.x, view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z);
		if(config.profiler_enabled)
			font->drawShadowed(int2(0, 60), Color::white, Color::black, "%s", prof_stats.c_str());

		swapBuffers();
		TextureCache::main_cache.nextFrame();

		profiler::updateTimer("main_loop", profiler::getTime() - loop_start);
		if(getTime() - stat_update_time > 0.25) {
			prof_stats = profiler::getStats();
			stat_update_time = getTime();
		}
		profiler::nextFrame();
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

