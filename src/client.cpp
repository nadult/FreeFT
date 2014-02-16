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

using namespace gfx;
using namespace game;
using namespace net;

static unique_ptr<World> world;

class Client: public net::LocalHost {
public:
	enum class Mode {
		disconnected,
		connecting,
		connected,
	};

	Client(int port)
		:LocalHost(Address(port)), m_mode(Mode::disconnected), m_actor_id(-1) {
			m_order.id = OrderId::invalid;
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

	void sendOrder(const Order &order) {
		m_order = order;
	}

	~Client() {
	}
	
	Mode mode() const { return m_mode; }

	void action() {
		if(m_server_id == -1)
			return;
		RemoteHost *host = getRemoteHost(m_server_id);

		beginFrame();
		beginSending(m_server_id);

		if(m_mode == Mode::connecting) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::join_accept) {
					//TODO: handle timeout
					
					host->verify(true);
					JoinAcceptPacket data;
					chunk >> data;

					m_actor_id = data.actor_id;

					if(world)
						delete world.release();
					world = unique_ptr<World>(new World(World::Mode::client, data.map_name.c_str()));
					EntityMap &emap = world->entityMap();
					for(int n =0; n < emap.size(); n++)
						if(emap[n].ptr)
							emap.remove(n);
					m_mode = Mode::connected;

					host->enqueChunk("", 0, ChunkType::join_complete, 0);
					printf("Joined to: %s (map: %s)\n", m_server_address.toString().c_str(), data.map_name.c_str());

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

			if(m_order.id != OrderId::invalid) {
				TempPacket temp;
				temp << m_order;
				host->enqueChunk(temp, ChunkType::actor_order, 0);
				m_order.id = OrderId::invalid;
			}
		}

		finishSending();
		finishFrame();
	}

	int actorId() const { return m_actor_id; }	

protected:
	void entityUpdate(InChunk &chunk) {
		DASSERT(chunk.type() == ChunkType::entity_full || chunk.type() == ChunkType::entity_delete);

		EntityMap &emap = world->entityMap();
		int entity_id = chunk.chunkId();

		if(entity_id >= 0) {
			if(entity_id < emap.size() && emap[entity_id].ptr)
				emap.remove(entity_id);

			if(chunk.type() == ChunkType::entity_full) {
				Entity *new_entity = Entity::construct(chunk);
				world->addEntity(entity_id, new_entity);
			}
		}
	}

private:
	int m_actor_id, m_server_id;
	Mode m_mode;

	Order m_order;
	Address m_server_address;
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
	
	Config config = loadConfig("client");
	ItemDesc::loadItems();

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);
	setWindowPos(config.window_pos);
	pollEvents();

	printDeviceInfo();
	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(-1000, 500);

	PFont font = Font::mgr["liberation_32"];

	host->connect(server_name, server_port);
	
	while(host->mode() != Client::Mode::connected) {
		host->action();
		sleep(0.01);
	}

	world->updateNaviMap(true);

	bool navi_show = 0;
	bool navi_debug = 0;
	bool shooting_debug = 1;
	bool entity_debug = 1;
	bool item_debug = 1;
	
	double last_time = getTime();
	vector<int3> path;
	int3 last_pos(0, 0, 0);
	float3 target_pos(0, 0, 0);

	int inventory_sel = -1, container_sel = -1;
	string prof_stats;
	double stat_update_time = getTime();

	while(host->mode() != Client::Mode::connected) {
		host->action();
		sleep(0.01);
	}

	while(pollEvents()) {
		double loop_start = profiler::getTime();
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		int actor_id = host->actorId();
		const Actor *actor = nullptr;
		if(actor_id >= 0 && actor_id < world->entityCount())
			actor = dynamic_cast<Actor*>(world->getEntity(actor_id));	

		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world->pixelIntersect(getMousePos() + view_pos,
				collider_tile_floors|collider_tile_roofs|collider_tile_stairs|collider_entities|visibility_flag);

		if(isect.isEmpty() || isect.isTile())
			isect = world->trace(ray, actor,
				collider_tile_floors|collider_tile_roofs|collider_tile_stairs|collider_entities|visibility_flag);
		
		Intersection full_isect = world->pixelIntersect(getMousePos() + view_pos, collider_all|visibility_flag);
		if(full_isect.isEmpty())
			full_isect = world->trace(ray, actor, collider_all|visibility_flag);
		
	//	if(isKeyDown('T') && !isect.isEmpty())
	//		actor->setPos(ray.at(isect.distance()));

		if(actor) {
			if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
				if(isect.entity() && entity_debug) {
					//isect.entity->interact(nullptr);
					InteractionMode mode = isect.entity()->entityType() == EntityId::item?
						interact_pickup : interact_normal;
					host->sendOrder(interactOrder(isect.entity(), mode));
				}
				else if(isect.isTile()) {
					//TODO: pixel intersect always returns distance == 0
					int3 wpos = int3(ray.at(isect.distance()) + float3(0, 0.5f, 0));
					host->sendOrder(moveOrder(wpos, !isKeyPressed(Key_lshift)));
				}
			}
			if(isMouseKeyDown(1) && shooting_debug) {
				host->sendOrder(attackOrder(0, (int3)target_pos));
			}
			if((navi_debug || (navi_show && !shooting_debug)) && isMouseKeyDown(1)) {
				int3 wpos = (int3)ray.at(isect.distance());
				path = world->findPath(last_pos, wpos);
				last_pos = wpos;
			}
			if(isKeyDown(Key_kp_add))
				host->sendOrder(changeStanceOrder(1));
			if(isKeyDown(Key_kp_subtract))
				host->sendOrder(changeStanceOrder(-1));
		}

		double time = getTime();
		if(!navi_debug)
			world->updateNaviMap(false);

		world->simulate((time - last_time) * config.time_multiplier);
		last_time = time;

		static int counter = 0;
		if(host)// && counter % 30 == 0)
			host->action();
		counter++;

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		if(actor)
			world->updateVisibility(actor->boundingBox());
		world->addToRender(renderer);

		renderer.addBox(isect.boundingBox(), Color::yellow);

		if(!full_isect.isEmpty() && shooting_debug && actor) {
			float3 target = ray.at(full_isect.distance());
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world->trace(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = shoot_isect.boundingBox();
				renderer.addBox(box, Color(255, 0, 0, 100));
				target_pos = shoot_ray.at(shoot_isect.distance());
			}
		}

		if(navi_debug || navi_show) {
			world->naviMap().visualize(renderer, true);
			world->naviMap().visualizePath(path, 3, renderer);
		}

		renderer.render();
		lookAt(view_pos);
			
		lookAt({0, 0});
		drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
		drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

		DTexture::bind0();
		drawQuad(0, 0, 250, config.profiler_enabled? 300 : 50, Color(0, 0, 0, 80));
		
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
		float3 isect_pos = ray.at(isect.distance());
		float3 actor_pos = actor? actor->pos() : float3(0, 0, 0);
		font->drawShadowed(int2(0, 0), Color::white, Color::black,
				"View:(%d %d)\nRay:(%.2f %.2f %.2f)\nActor:(%.2f %.2f %.2f)",
				view_pos.x, view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z, actor_pos.x, actor_pos.y, actor_pos.z);
		if(config.profiler_enabled)
			font->drawShadowed(int2(0, 60), Color::white, Color::black, "%s", prof_stats.c_str());

		if(item_debug && actor) {
			if(isKeyPressed(Key_lctrl)) {
				if(isKeyDown(Key_up))
					container_sel--;
				if(isKeyDown(Key_down))
					container_sel++;
			}
			else {
				if(isKeyDown(Key_up))
					inventory_sel--;
				if(isKeyDown(Key_down))
					inventory_sel++;
			}

			Container *container = dynamic_cast<Container*>(isect.entity());
			if(container && !(container->isOpened() && areAdjacent(*actor, *container)))
				container = nullptr;

			inventory_sel = clamp(inventory_sel, -2, actor->inventory().size() - 1);
			container_sel = clamp(container_sel, 0, container? container->inventory().size() - 1 : 0);

			if(isKeyDown('D') && inventory_sel >= 0)
				host->sendOrder(dropItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel >= 0)
				host->sendOrder(equipItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel < 0) {
				InventorySlotId::Type slot_id = InventorySlotId::Type(-inventory_sel - 1);
				host->sendOrder(unequipItemOrder(slot_id));
			}

			if(container) {
				if(isKeyDown(Key_right) && inventory_sel >= 0)
					host->sendOrder(transferItemOrder(container, transfer_to, inventory_sel, 1));
				if(isKeyDown(Key_left))
					host->sendOrder(transferItemOrder(container, transfer_from, container_sel, 1));
			}

			string inv_info = actor->inventory().printMenu(inventory_sel);
			string cont_info = container? container->inventory().printMenu(container_sel) : string();
				
			IRect extents = font->evalExtents(inv_info.c_str());
			font->drawShadowed(int2(0, config.resolution.y - extents.height()),
							Color::white, Color::black, "%s", inv_info.c_str());

			extents = font->evalExtents(cont_info.c_str());
			font->drawShadowed(int2(config.resolution.x - extents.width(), config.resolution.y - extents.height()),
							Color::white, Color::black, "%s", cont_info.c_str());
		}

		swapBuffers();
		TextureCache::main_cache.nextFrame();

		profiler::updateTimer("main_loop", profiler::getTime() - loop_start);
		if(getTime() - stat_update_time > 0.25) {
			prof_stats = profiler::getStats();
			stat_update_time = getTime();
		}
		profiler::nextFrame();
	}

	host->disconnect();

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

