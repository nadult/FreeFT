/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "net/client.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "net/host.h"
#include "net/lobby.h"
#include "net/socket.h"

using namespace game;
using namespace net;

namespace net {

	Client::Client(int port)
	  :LocalHost(Address(port)), m_mode(Mode::disconnected) {
		m_order_type = OrderTypeId::invalid;
	}
		
	void Client::connect(Address address) {
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

	void Client::disconnect() {
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

	Client::~Client() {
	}
	
	void Client::beginFrame() {
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

	void Client::finishFrame() {
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
	
	void Client::entityUpdate(InChunk &chunk) {
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

	void Client::replicateOrder(POrder &&order, EntityRef entity_ref) {
		//TODO: handle cancel_prev
		if(entity_ref == m_actor_ref) {
			m_order_type = order->typeId();
			m_orders.emplace_back(std::move(order));
			m_order_send_time = getTime();
		}
	}

}

