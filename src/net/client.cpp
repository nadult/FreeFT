// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include <memory.h>
#include <cstdio>
#include <fwk/sys/rollback.h>

#include "net/client.h"
#include "game/actor.h"
#include "game/world.h"
#include "net/host.h"
#include "net/socket.h"

#include "game/death_match.h"

using namespace game;
using namespace net;

namespace net {

	Client::Client(int port)
	  :LocalHost(Address(port? port : net::randomPort())), m_mode(Mode::disconnected), m_server_id(-1), m_client_id(-1) {
	}
		
	bool Client::getLobbyData(vector<ServerStatusChunk> &out) {
		InPacket packet;
		bool any_packets = false;

		while(getLobbyPacket(packet)) {
			// TODO: verify that this is ok
			RollbackContext::begin([&]() {
				LobbyChunkId chunk_id;
				packet >> chunk_id;

				if(chunk_id == LobbyChunkId::server_list) {
					while(packet.pos() < packet.size()) {
						ServerStatusChunk chunk;
						packet >> chunk;

						if(chunk.address.isValid())
							out.push_back(chunk);
					}
				}
				any_packets = true;
			});
		}

		return any_packets;
	}

	void Client::requestLobbyData() {
		OutPacket request(0, -1, -1, PacketInfo::flag_lobby);
		request << LobbyChunkId::server_list_request;
		sendLobbyPacket(request);
	}
		
	void Client::connect(Address address, const string &nick_name, const string &password) {
		if(m_mode != Mode::disconnected)
			disconnect();

		{
			OutPacket punch_through(0, -1, -1, PacketInfo::flag_lobby);
			punch_through << LobbyChunkId::join_request << address.ip << address.port;
			sendLobbyPacket(punch_through);
		}

		m_nick_name = nick_name;
		m_server_address = address;
		m_server_id = addRemoteHost(m_server_address, -1);
		if(m_server_id != -1) {
			RemoteHost *host = getRemoteHost(m_server_id);
			TempPacket chunk;
			chunk << m_nick_name << password;
			host->enqueChunk(chunk, ChunkType::join, 0);
			m_mode = Mode::connecting;
		}
	}

	void Client::disconnect() {
		if(m_server_id != -1) {
			RemoteHost *host = getRemoteHost(m_server_id);
			if(host) {
				beginSending(m_server_id);
				host->enqueUChunk("", 0, ChunkType::leave, 0, 0);
				finishSending();
				removeRemoteHost(m_server_id);
				m_server_id = -1;
			}

			m_server_id = -1;
			m_client_id = -1;
			m_mode = Mode::disconnected;
		}
	}

	Client::~Client() {
		disconnect();
	}
		
	void Client::setWorld(game::PWorld world) {
		DASSERT(m_mode == Mode::waiting_for_world_update);

		if(world->mapName() == m_level_info.map_name) {
			if(m_level_info.game_mode == GameModeId::death_match)
				world->assignGameMode<DeathMatchClient>(m_client_id, m_nick_name);
			world->setReplicator(this);
			m_world = world;
			m_mode = Mode::world_updated;
		}
	}

	void Client::beginFrame() {
		LocalHost::receive();
		
		if(m_server_id == -1)
			return;
		RemoteHost *host = getRemoteHost(m_server_id);

		if(m_mode == Mode::connecting) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::join_accept) {
					host->verify(true);
					m_client_id = decodeInt(chunk);
					chunk >> m_level_info;
					m_mode = Mode::waiting_for_world_update;
				}
				else if(chunk.type() == ChunkType::join_refuse) {
					chunk >> m_refuse_reason;
					removeRemoteHost(m_server_id);
					m_server_id = -1;
					m_mode = Mode::refused;
				}
			}
		}

		if(m_mode == Mode::world_updated) {
			host->enqueChunk("", 0, ChunkType::level_loaded, 0);
			m_mode = Mode::playing;
		}

		if(m_mode == Mode::playing) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::entity_delete || chunk.type() == ChunkType::entity_full)
					entityUpdate(chunk);
				else if(chunk.type() == ChunkType::level_info) {
					LevelInfoChunk new_info;
					chunk >> new_info;
					if(new_info.map_name != m_level_info.map_name)
						m_mode = Mode::waiting_for_world_update;
					m_level_info = new_info;

					//TODO: message about changed map first?
					m_world.reset();
					break;
				}
				else if(chunk.type() == ChunkType::message)
					m_world->onMessage(chunk, -1);
			}
		}
	}

	void Client::finishFrame() {
		m_timestamp++;

		if(m_server_id == -1)
			return;

		RemoteHost *host = getRemoteHost(m_server_id);
		beginSending(m_server_id);

		if(m_mode == Mode::playing) {
			while( const Chunk *chunk_ptr  = host->getIChunk() ) {
				InChunk chunk(*chunk_ptr);

				if(chunk.type() == ChunkType::entity_delete || chunk.type() == ChunkType::entity_full)
					entityUpdate(chunk);
			}
		}

		finishSending();

		if(host->timeout() > double(timeout)) {
			disconnect();
			m_mode = Mode::timeout;
		}
	}
	
	void Client::entityUpdate(InChunk &chunk) {
		DASSERT(chunk.type() == ChunkType::entity_full || chunk.type() == ChunkType::entity_delete);
		if(!m_world || m_mode != Mode::playing)
			return;

		m_world->removeEntity(m_world->toEntityRef(chunk.chunkId()));

		if(chunk.type() == ChunkType::entity_full) {
			Entity *new_entity = Entity::construct(chunk);
			m_world->addEntity(PEntity(new_entity), chunk.chunkId());
		}
	}
		
	void Client::sendMessage(net::TempPacket &packet, int target_id) {
		DASSERT(target_id == -1);
		RemoteHost *server = getRemoteHost(m_server_id);
		if(server)
			server->enqueChunk(packet.data(), packet.pos(), ChunkType::message, 1);
	}

}

