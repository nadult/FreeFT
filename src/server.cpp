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
#include <algorithm>

using namespace gfx;
using namespace game;

float frand() {
	return float(rand()) / float(RAND_MAX);
}
	
using namespace net;




class Server: public net::Host {
public:
	Server(int port) :Host(Address(port)), m_world(0), m_timestamp(0), m_client_count(0) { }

	bool isConnected() const { return !m_clients.empty(); }

	enum {
		max_clients = 32,
		client_timeout = 10,
	};

	int spawnActor(const float3 &pos) {
		DASSERT(m_world);
		return m_world->addEntity(new Actor(ActorTypeId::male, pos));
	}

	void onJoin(InPacket &packet, const Address &source) {
		int client_id = -1;

		if(m_client_count == max_clients) {
			OutPacket packet(0, m_timestamp, 0, 0);
			packet << SubPacketType::join_refuse << RefuseReason::too_many_clients;
			send(packet, source);
			return;
		}

		bool is_added = false;
		for(int c = 0; c < (int)m_clients.size(); c++)
			if(m_clients[c].address == source) {
				client_id = c;
				is_added = true;
				break;
			}

		if(!is_added) {
			for(int c = 0; c < (int)m_clients.size(); c++)
				if(!m_clients[c].isValid()) {
					client_id = c;
					break;
				}
			if(client_id == -1) {
				m_clients.push_back(Client());
				client_id = (int)m_clients.size() - 1;
			}

			const EntityMap &map = m_world->entityMap();
			
			Client &client = m_clients[client_id];
			client = Client(source);
			client.actor_id = spawnActor(float3(245 + frand() * 10.0f, 128, 335 + frand() * 10.0f));
			client.last_packet_time = m_current_time;
			printf("Client connected (cid:%d): %s\n", (int)client_id, source.toString().c_str());

			client.update_map.resize(map.size() * 2);
			for(int n = 0; n < map.size(); n++)
				if(map[n].ptr)
					client.update_map[n] = true;
			m_client_count++;
		}

		printf("Sending accept\n");
		Client &client = m_clients[client_id];
		OutPacket opacket(client.packet_id++, m_timestamp, client_id, 0);
		opacket << SubPacketType::join_accept;
		opacket << JoinAcceptPacket{string(m_world->mapName()), client.actor_id};
		send(opacket, source);
	}

	void disconnectClient(int client_id) {
		Client &client = m_clients[client_id];
		m_world->removeEntity(client.actor_id);
		printf("Client disconnected: %s\n", client.address.toString().c_str());

		client.clear();
		m_client_count--;
	}

	// Assuming that there is enough space in the packet
	void appendAcks(vector<SeqNumber> &acks, OutPacket &packet) {
		int num_acks = min(16, (int)acks.size());
		//TODO: send ack range

		packet << SubPacketType::ack;
		packet.encodeInt(num_acks);
		packet.saveData(&acks[0], sizeof(acks[0]) * num_acks);
		acks.erase(acks.begin(), acks.begin() + num_acks);
	}

	void replicateWorld() {
		EntityMap &emap = m_world->entityMap();

		vector<int> &new_updates = m_world->replicationList();
		for(int n = 0; n < (int)m_clients.size(); n++) {
			Client &client = m_clients[n];
			if(emap.size() > client.update_map.size())
				client.update_map.resize(emap.size() * 2);

			for(int n = 0; n < (int)new_updates.size(); n++)
				client.update_map[new_updates[n]] = true;
		}
		new_updates.clear();

		for(int n = 0; n < (int)m_clients.size(); n++) {
			Client &client = m_clients[n];
			int idx = 0;

			//TODO: priority for objects visible by client
			for(int p = 0; p < client.max_packets; p++) {
				OutPacket packet(client.packet_id++, m_timestamp, n, PacketInfo::flag_need_ack);

				if(p == 0)
					appendAcks(client.in_acks, packet);

				BitVector &map = client.update_map;
				while(idx < emap.size()) {
					if(!map.any(idx >> BitVector::base_shift)) {
						idx = ((idx >> BitVector::base_shift) + 1) << BitVector::base_shift;
						continue;
					}
					if(map[idx]) {
						const Entity *entity = emap[idx].ptr;
	
						char sub_packet[PacketInfo::max_size];
						MemorySaver substream(sub_packet, sizeof(sub_packet));

						substream << (entity? SubPacketType::entity_full : SubPacketType::entity_delete);
						substream.encodeInt(idx);
						if(entity)
							substream << entity->entityType() << *entity;

						if(packet.spaceLeft() < substream.pos())
							break;

						packet.saveData(sub_packet, substream.pos());
						map[idx] = false;
						//TODO: update non_acked list
					}

					idx++;
				}

				if(packet.size() == PacketInfo::header_size)
					break; // Nothing more to send

				send(packet, client.address);
				client.packet_id++;
			}
		}
	}

	static void dropMsg(InPacket &packet, Address &source, const char *assert) {
		printf("Dropping packet (%d bytes) from: %s (%s failed)\n", (int)packet.size(), source.toString().c_str(), assert);
	}

	void action() {
		InPacket packet;
		Address source;

		double time = getTime();
		m_current_time = time;

		while(receive(packet, source)) {
			//printf("Packet: %d bytes\n", packet.size());
			//TODO: dealing with fucked-up packets
			//TODO: reporting fucked-up data ?

#define VERIFY(...)	{ if(!(__VA_ARGS__)) { dropMsg(packet, source, #__VA_ARGS__); goto DROP_PACKET; } }

			int client_id = packet.clientId();
			Client *client = nullptr;

			if(client_id == -1 && !packet.end()) {
				SubPacketType sub_type;
				packet >> sub_type;
				VERIFY(sub_type == SubPacketType::join);
				onJoin(packet, source);
				continue;
			}

			VERIFY(client_id >= 0 && client_id < (int)m_clients.size());

			//TODO: make client verification more secure
			client = &m_clients[client_id];
			VERIFY(client->isValid() && source == client->address);
			client->last_packet_time = time;
			client->in_acks.push_back(packet.packetId());

			while(!packet.end()) {
				SubPacketType sub_type;
				packet >> sub_type;

				if(sub_type == SubPacketType::leave) {
					disconnectClient(client_id);
				}
				else if(sub_type == SubPacketType::ack) {
					int count = packet.decodeInt();
					for(int n =0; n < count; n++) {
						SeqNumber seq;
						packet >> seq;
					}
					//TODO: finish
				}
				else if(sub_type == SubPacketType::actor_order) {
					Actor *actor = dynamic_cast<Actor*>(m_world->getEntity(client->actor_id));
					Order order;
					packet >> order;
					actor->setNextOrder(order);
				}
				else {
					VERIFY(0 && "Unknown sub-packet type");
				}

				break;
			}

			continue;
DROP_PACKET:;
#undef VERIFY
		}

		if(m_world)
			replicateWorld();

		for(int n = 0; n < (int)m_clients.size(); n++) {
			Client &client = m_clients[n];
			if(client.isValid() && time - client.last_packet_time > (double)client_timeout)
				disconnectClient(n);
		}


		m_timestamp++;
	}

	struct Client {
		Client() { }
		Client(Address addr) :address(addr), in_packet_id(-1), packet_id(0), actor_id(-1), last_packet_time(-1.0) {
			in_acks.reserve(256);
			max_packets = 4;
		}

		bool isValid() const { return address.isValid(); }
		void clear() { *this = Client(); }


		struct NotAckedUpdates {
			vector<int> entity_ids;
			SeqNumber seq_num;
		};

		vector<SeqNumber> in_acks;
		vector<NotAckedUpdates> not_acked_updates;
		BitVector update_map;

		Address address;
		int in_packet_id, packet_id, actor_id;
		int max_packets;
		double last_packet_time;
	};

	void setWorld(World *world) { m_world = world; }

private:
	vector<Client> m_clients;
	World *m_world;
	SeqNumber m_timestamp;
	int m_client_count;
	double m_current_time;
};

int safe_main(int argc, char **argv)
{
	int port = 0;
	string map_name = "data/maps/mission05.mod";

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

	World world(World::Mode::server, map_name.c_str());

	world.updateNaviMap(true);
	host->setWorld(&world);

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
		Intersection isect = world.pixelIntersect(getMousePos() + view_pos, collider_all|visibility_flag);
		if(isect.isEmpty())
			isect = world.trace(ray, nullptr, collider_all|visibility_flag);

		double time = getTime();

		world.updateNaviMap(false);
		world.simulate((time - last_time) * config.time_multiplier);
		last_time = time;

		static int counter = 0;
		if(host)// && counter % 4 == 0)
			host->action();
		counter++;
		

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);
		
		if(!isect.isEmpty()) {		
			FBox box = isect.boundingBox();
			renderer.addBox(box, Color(255, 255, 255, 100));
		}

	//	world.updateVisibility(actor->boundingBox());
		world.addToRender(renderer);

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

