/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "navi_map.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/trigger.h"
#include "net/lobby.h"
#include "net/server.h"
#include <list>
#include <algorithm>

using namespace gfx;
using namespace game;
using namespace net;

namespace net {

	Server::Server(int port) :LocalHost(Address(port)), m_client_count(0) {
		m_lobby_timeout = m_current_time = getTime();
	}

	Server::~Server() {
		//TODO: inform clients that server is closing

		try {
			OutPacket out(0, -1, -1, PacketInfo::flag_lobby);
			out << LobbyChunkId::server_down;
			sendLobbyPacket(out);
		}
		catch(...) { }
	}

	EntityRef Server::spawnActor(EntityRef spawn_zone_ref) {
		DASSERT(m_world);

		const Trigger *spawn_zone = m_world->refEntity<Trigger>(spawn_zone_ref);
		DASSERT(spawn_zone);

		PEntity actor = (PEntity)new Actor(getProto("male", ProtoId::actor));

		FBox spawn_box = spawn_zone->boundingBox();
		float3 spawn_pos = spawn_box.center();

		actor->setPos(spawn_pos);
		while(!m_world->findAny(actor->boundingBox(), {Flags::all | Flags::colliding})) {
			spawn_pos.y -= 1.0f;
			actor->setPos(spawn_pos);
		}

		spawn_pos.y += 1.0f;
		actor->setPos(spawn_pos);

		return m_world->addEntity(std::move(actor));
	}

	void Server::disconnectClient(int client_id) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		m_clients[client_id].mode = ClientMode::to_be_removed;
	}

	void Server::handleHostReceiving(RemoteHost &host, Client &client) {
		DASSERT(client.isValid());

		const Chunk *chunk = nullptr;

		while( const Chunk *chunk_ptr = host.getIChunk() ) {
			InChunk chunk(*chunk_ptr);

			if(chunk.type() == ChunkType::join) {
				vector<Trigger*> spawn_zones;
				for(int n = 0; n < m_world->entityCount(); n++) {
					Trigger *trigger = m_world->refEntity<Trigger>(n);
					if(trigger && trigger->classId() == TriggerClassId::spawn_zone)
						spawn_zones.push_back(trigger);
				}

				client.actor_ref = spawnActor(spawn_zones[rand() % spawn_zones.size()]->ref());
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

	void Server::handleHostSending(RemoteHost &host, Client &client) {
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

	void Server::beginFrame() {
		InPacket packet;
		Address source;

		double time = getTime();
		m_current_time = time;

		LocalHost::receive();

		//TODO: handle lobby packets

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

	void Server::finishFrame() {
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

		if(m_current_time >= m_lobby_timeout) {
			OutPacket out(0, -1, -1, PacketInfo::flag_lobby);
			ServerStatusChunk chunk;
			chunk.address = Address();
			chunk.map_name = "map_name";
			chunk.server_name = "test_serv";
			chunk.num_players = m_client_count;
			chunk.max_players = max_remote_hosts;
			chunk.game_mode = GameMode::death_match;
			out << LobbyChunkId::server_status << chunk;
			sendLobbyPacket(out);
			m_lobby_timeout = m_current_time + 10.0;
		}
	}

	void Server::createWorld(const string &file_name) {
		//TODO: update clients
		m_world = new World(file_name, World::Mode::server, this);
	}

	void Server::replicateEntity(int entity_id) {
		m_replication_list.emplace_back(entity_id);
	}

}

