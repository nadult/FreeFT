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
#include "sys/network.h"
#include <list>

using namespace gfx;
using namespace game;

float frand() {
	return float(rand()) / float(RAND_MAX);
}

namespace PacketType {
	enum Type: char {
		host_info = 0,
	};
};

struct PacketHeader {
	int packet_id;
	int timestamp;
	int packet_type;
};




class Host {
public:
	Host(const net::Address &address) :m_socket(address), m_world(nullptr) { }

	void setWorld(World *world) { m_world = world; }

	virtual ~Host() { }
	virtual void action() = 0;
	virtual bool isConnected() const = 0;


protected:
	World *m_world;
	net::Socket m_socket;
};

class Server: public Host {
public:
	Server(int port) :Host(net::Address(0, port)) { }

	bool isConnected() const { return !m_clients.empty(); }

	void sendPacket(char *data, int size, int client_id) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		DASSERT(data && size >= (int)sizeof(PacketHeader));

		Client &client = m_clients[client_id];
		PacketHeader header;
		header.packet_id = client.packet_id++;
		memcpy(data, &header, sizeof(header));
		m_socket.send(data, size, client.address);
	}

	void action() {
		char buffer[2048 + sizeof(PacketHeader)];

		while(true) {
			net::Address source;
			int ret = m_socket.receive(buffer, sizeof(buffer), source);
			if(ret > 0) {
				printf("Client connected: %s\n", source.toString().c_str());
				m_clients.push_back(Client(source));
			}
			if(!ret)
				break;
		}

		if(m_world) {
			EntityMap &emap = m_world->entityMap();
			int packet_size = 0;

			for(int n = 0; n < emap.size(); n++) {
				Actor *actor = dynamic_cast<Actor*>(emap[n].ptr);
				if(actor && actor->actorType() == ActorTypeId::male) {
					DataStream stream(buffer + sizeof(PacketHeader), sizeof(buffer) - sizeof(PacketHeader), false);
					actor->save(stream);
					packet_size = stream.pos() + sizeof(PacketHeader);
					break;
				}
			}

			if(packet_size) {
				for(int n = 0; n < (int)m_clients.size(); n++) {
				//	printf("Sending data packet (%d bytes) to: %s\n", packet_size, m_clients[n].address.toString().c_str());
					sendPacket(buffer, packet_size, n);
				}
			}
		}
	}

	struct Client {
		Client(net::Address addr) :address(addr), packet_id(0) { }

		net::Address address;
		int packet_id;
	};

private:
	vector<Client> m_clients;
};

struct Packet {
	vector<char> data;
};

class Client: public Host {
public:
	Client(int port, const char *server_name, int server_port)
		:Host(net::Address(0, port)), m_server_address(server_name, server_port), m_last_packet_id(-1) {

		char data[256];
		sprintf(data, "Hi from client\n");
		m_socket.send(data, 256, m_server_address);
	}
	
	bool isConnected() const { return true; }

	void action() {
		while(true) {
			char buffer[2048];
			net::Address source;
			int ret = m_socket.receive(buffer, sizeof(buffer), source);
			if(!ret)
				break;

			Packet packet;
			packet.data.resize(ret);
			memcpy(packet.data.data(), buffer, ret);
			m_packets.push_back(packet);
		}

		int list_size = m_packets.size();
		if(list_size > 2) {
			printf("Dropping %d packets\n", list_size - 2);
		}

		while(list_size > 2) {
			m_packets.pop_front();
			list_size--;
		}

		if(!m_packets.empty() && m_world) {
			vector<char> packet = m_packets.front().data;
			m_packets.pop_front();

			PacketHeader header;
			memcpy(&header, packet.data(), sizeof(header));

			if(m_last_packet_id != -1 && header.packet_id != m_last_packet_id + 1)
				printf("Packet dropped! (got: %d last: %d)\n", header.packet_id, m_last_packet_id);
			m_last_packet_id = header.packet_id;

		//	printf("Received: packet %d (%d bytes) from: %s\n", header.packet_id, ret, source.toString().c_str());
			EntityMap &emap = m_world->entityMap();

			for(int n = 0; n < emap.size(); n++) {
				Actor *actor = dynamic_cast<Actor*>(emap[n].ptr);
				if(actor && actor->actorType() == ActorTypeId::male) {
					emap.remove(actor);
					break;
				}
			}

			DataStream stream(packet.data() + sizeof(header), packet.size() - sizeof(header), true);
			Entity *new_actor = Entity::construct(stream);
			m_world->addEntity(new_actor);
		}
	}

private:
	std::list<Packet> m_packets;
	net::Address m_server_address;
	int m_last_packet_id;
};

int safe_main(int argc, char **argv)
{
	unique_ptr<Host> host = 0;
	string map_name = "data/maps/mission05.mod";

	for(int n = 1; n < argc; n++) {
		if(strcmp(argv[n], "-h") == 0) {
			ASSERT(n + 1 < argc);
			int port = atoi(argv[++n]);
			host = unique_ptr<Host>(new Server(port));
		}
		else if(strcmp(argv[n], "-c") == 0) {
			ASSERT(n + 3 < argc);
			int port = atoi(argv[++n]);
			const char *server_name = argv[++n];
			int server_port = atoi(argv[++n]);
			host = unique_ptr<Host>(new Client(port, argv[++n], server_port));
		}
		else if(strcmp(argv[n], "-m") == 0) {
			ASSERT(n + 1 < argc);
			map_name = string("data/maps/") + argv[++n];
		}
	}

	if(!host) {
		printf("Synopsis:\n");
		printf("%s -h port\n", argv[0]);
		printf("%s -c port server_address server_port\n", argv[0]);
		return 0;
	}
	
	Config config = loadConfig("game");
	ItemDesc::loadItems();

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);

	printDeviceInfo();
	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(-1000, 500);

	PFont font = Font::mgr["liberation_32"];

	World world(map_name.c_str());

	Actor *actor = world.addEntity(new Actor(ActorTypeId::male, float3(245, 128, 335)));
	world.updateNaviMap(true);
	host->setWorld(&world);

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

	while(!host->isConnected()) {
		host->action();
		sleep(0.01);
	}

	while(pollEvents()) {
		double loop_start = profiler::getTime();
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world.pixelIntersect(getMousePos() + view_pos,
				collider_tile_floors|collider_tile_roofs|collider_entities|visibility_flag);
		if(isect.isEmpty())
			isect = world.trace(ray, actor,
				collider_tile_floors|collider_tile_roofs|collider_entities|visibility_flag);
		
		Intersection full_isect = world.pixelIntersect(getMousePos() + view_pos, collider_all|visibility_flag);
		if(full_isect.isEmpty())
			full_isect = world.trace(ray, actor, collider_all|visibility_flag);

		
		if(isKeyDown('T') && !isect.isEmpty())
			actor->setPos(ray.at(isect.distance()));

		if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
			if(isect.entity() && entity_debug) {
				//isect.entity->interact(nullptr);
				InteractionMode mode = isect.entity()->entityType() == EntityId::item?
					interact_pickup : interact_normal;
				actor->setNextOrder(interactOrder(isect.entity(), mode));
			}
			else if(navi_debug) {
				//TODO: do this on floats, in actor and navi code too
				int3 wpos = (int3)(ray.at(isect.distance()));
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));

			}
			else if(isect.isTile()) {
				//TODO: pixel intersect always returns distance == 0
				int3 wpos = int3(ray.at(isect.distance()) + float3(0, 0.5f, 0));
				actor->setNextOrder(moveOrder(wpos, !isKeyPressed(Key_lshift)));
			}
		}
		if(isMouseKeyDown(1) && shooting_debug) {
			actor->setNextOrder(attackOrder(0, (int3)target_pos));
		}
		if((navi_debug || (navi_show && !shooting_debug)) && isMouseKeyDown(1)) {
			int3 wpos = (int3)ray.at(isect.distance());
			path = world.findPath(last_pos, wpos);
			last_pos = wpos;
		}
		if(isKeyDown(Key_kp_add))
			actor->setNextOrder(changeStanceOrder(1));
		if(isKeyDown(Key_kp_subtract))
			actor->setNextOrder(changeStanceOrder(-1));

		if(isKeyDown('R') && navi_debug) {
			world.naviMap().removeColliders();
		}

		double time = getTime();
		if(!navi_debug)
			world.updateNaviMap(false);

		world.simulate((time - last_time) * config.time_multiplier);
		last_time = time;

		static int counter = 0;
		if(host)// && counter % 4 == 0)
			host->action();
		counter++;

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		world.updateVisibility(actor->boundingBox());

		world.addToRender(renderer);

		if((entity_debug && isect.isEntity()) || 1)
			renderer.addBox(isect.boundingBox(), Color::yellow);

		if(!full_isect.isEmpty() && shooting_debug) {
			float3 target = ray.at(full_isect.distance());
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world.trace(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = shoot_isect.boundingBox();
				renderer.addBox(box, Color(255, 0, 0, 100));
				target_pos = shoot_ray.at(shoot_isect.distance());
			}
		}

		if(navi_debug || navi_show) {
			world.naviMap().visualize(renderer, true);
			world.naviMap().visualizePath(path, 3, renderer);
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
		float3 actor_pos = actor->pos();
		font->drawShadowed(int2(0, 0), Color::white, Color::black,
				"View:(%d %d)\nRay:(%.2f %.2f %.2f)\nActor:(%.2f %.2f %.2f)",
				view_pos.x, view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z, actor_pos.x, actor_pos.y, actor_pos.z);
		if(config.profiler_enabled)
			font->drawShadowed(int2(0, 60), Color::white, Color::black, "%s", prof_stats.c_str());

		if(item_debug) {
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
				actor->setNextOrder(dropItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel >= 0)
				actor->setNextOrder(equipItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel < 0) {
				InventorySlotId::Type slot_id = InventorySlotId::Type(-inventory_sel - 1);
				actor->setNextOrder(unequipItemOrder(slot_id));
			}

			if(container) {
				if(isKeyDown(Key_right) && inventory_sel >= 0)
					actor->setNextOrder(transferItemOrder(container, transfer_to, inventory_sel, 1));
				if(isKeyDown(Key_left))
					actor->setNextOrder(transferItemOrder(container, transfer_from, container_sel, 1));
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

	host->setWorld(nullptr);

/*	PTexture atlas = TextureCache::main_cache.atlas();
	Texture tex;
	atlas->download(tex);
	Saver("atlas.tga") & tex; */

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

