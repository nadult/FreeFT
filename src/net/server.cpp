/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "net/server.h"

#include <memory.h>
#include <cstdio>
#include <list>
#include <algorithm>

#include "game/world.h"
#include "game/game_mode.h"
#include "game/death_match.h"
#include "navi_map.h"

using namespace game;
using namespace net;

namespace net {
		
	ServerConfig::ServerConfig()
		:m_console_mode(false), m_port(0), m_max_players(16) {
	}

	ServerConfig::ServerConfig(const XMLNode &node) :ServerConfig() {
		using namespace xml_conversions;

		if(const char *attrib = node.hasAttrib("max_players")) {
			m_max_players = fromString<int>(attrib);
			ASSERT(m_max_players >= 1 && m_max_players <= Server::max_remote_hosts);
		}
		if(const char *attrib = node.hasAttrib("console_mode"))
			m_console_mode = fromString<bool>(attrib);
		if(const char *attrib = node.hasAttrib("port")) {
			m_port = fromString<int>(attrib);
			ASSERT(m_port > 0 && m_port < 65536);
		}
		if(const char *attrib = node.hasAttrib("password"))
			m_password = attrib;
		m_map_name = node.attrib("map_name");
		m_server_name = node.attrib("name");

		ASSERT(m_server_name.size() >= 4 && m_server_name.size() <= 20);
		ASSERT(isValid());
	}
		
	bool ServerConfig::isValid() const {
		return !m_server_name.empty() && !m_map_name.empty();
	}

	void ServerConfig::save(XMLNode &node) {
		node.addAttrib("name", node.own(m_server_name));
		node.addAttrib("map_name", node.own(m_map_name));
		node.addAttrib("port", m_port);
		node.addAttrib("max_players", m_max_players);
		node.addAttrib("console_mode", m_console_mode);
		node.addAttrib("password", node.own(m_password));
	}

	Server::Server(const ServerConfig &config) :LocalHost(Address(m_config.m_port)), m_config(config), m_game_mode(nullptr) {
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
		
	int Server::numActiveClients() const {
		int out = 0;
		for(int n = 0; n < (int)m_clients.size(); n++)
			if(m_clients[n].isValid())
				out++;
		return out;
	}

	void Server::disconnectClient(int client_id) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		m_clients[client_id].mode = ClientMode::to_be_removed;
		m_clients[client_id].notify_others = true;
		m_clients[client_id].nick_name = string();
	}
		
	void Server::handleHostReceiving(RemoteHost &host, int client_id) {
		ClientInfo &client = m_clients[client_id];

		while( const Chunk *chunk_ptr = host.getIChunk() ) {
			InChunk chunk(*chunk_ptr);

			if(chunk.type() == ChunkType::join && client.mode == ClientMode::connecting) {
				chunk >> client.nick_name;
				string password;
				chunk >> password;

				bool disconnect = false;
				RefuseReason refuse_reason;

				if(!m_config.m_password.empty() && password != m_config.m_password) {
					refuse_reason = RefuseReason::wrong_password;
					disconnect = true;
				}
				else if(numActiveClients() > maxPlayers()) {
					refuse_reason = RefuseReason::server_full;
					disconnect = true;
				}
				else for(int n = 0; n < (int)m_clients.size(); n++)
					if(n != client_id && m_clients[n].isValid() && m_clients[n].nick_name == client.nick_name) {
						refuse_reason = RefuseReason::nick_already_used;
						disconnect = true;
						break;
					}
				
				if(disconnect) {	
					TempPacket temp;
					temp << refuse_reason;
					host.enqueChunk(temp, ChunkType::join_refuse, 0);
					disconnectClient(client_id);
					break;
				}

				client.is_loading_level = true;
				client.update_map.resize(m_world->entityCount() * 2);
				for(int n = 0; n < m_world->entityCount(); n++)
					if(m_world->refEntity(n))
						client.update_map[n] = true;

				TempPacket temp;
				temp.encodeInt(client_id);
				temp << LevelInfoChunk{ m_world->mapName(), m_world->gameModeId() };
				host.enqueChunk(temp, ChunkType::join_accept, 0);

				printf("Client connected (%d / %d): %s (%s)\n", numActiveClients(), maxPlayers(),
						client.nick_name.c_str(), host.address().toString().c_str());
			}
			else if(chunk.type() == ChunkType::level_loaded && client.mode == ClientMode::connecting) {	
				m_game_mode->onClientConnected(client_id, client.nick_name);

				//TODO: timeout for level_loaded?
				client.mode = ClientMode::connected;
				client.is_loading_level = false;
				client.notify_others = true;
				host.verify(true);
				break;
			}
			else if(chunk.type() == ChunkType::leave) {	
				disconnectClient(client_id);
				break;
			}
			else if(chunk.type() == ChunkType::message && client.mode == ClientMode::connected) {
				m_world->onMessage(chunk, client_id);
			}
		}
	}

	void Server::handleHostSending(RemoteHost &host, int client_id) {
		ClientInfo &client = m_clients[client_id];
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
		if(!m_world)
			return;

		InPacket packet;
		Address source;

		double time = getTime();
		m_current_time = time;

		LocalHost::receive();

		while(getLobbyPacket(packet)) {
			LobbyChunkId id;
			packet >> id;
			Address target;
			packet >> target.ip >> target.port;

			if(id == LobbyChunkId::join_request && target.isValid()) {
				OutPacket punch(0, -1, -1, PacketInfo::flag_lobby);
				punch << LobbyChunkId::punch_through;
				m_socket.send(punch, target);
			}
		}

		for(int h = 0; h < numRemoteHosts(); h++) {
			RemoteHost *host = getRemoteHost(h);

			if(!host)
				continue;
				
			if((int)m_clients.size() <= h)
				m_clients.resize(h + 1);
			ClientInfo &client = m_clients[h];
			if(!client.isValid()) {
				client = ClientInfo();
				client.mode = ClientMode::connecting;
				client.host_id = h;
			}
			if(m_world->entityCount() > client.update_map.size())
				client.update_map.resize(m_world->entityCount() * 2);

			handleHostReceiving(*host, h);
		}
	}

	void Server::finishFrame() {
		if(!m_world)
			return;
		m_timestamp++;

		for(int h = 0; h < (int)m_clients.size(); h++)
			if(m_clients[h].mode == ClientMode::to_be_removed)
				m_game_mode->onClientDisconnected(h);
		
		for(int h = 0; h < numRemoteHosts(); h++) {
			RemoteHost *host = getRemoteHost(h);

			if(!host || h >= (int)m_clients.size())
				continue;
				
			ClientInfo &client = m_clients[h];
			if(m_world->entityCount() > client.update_map.size())
				client.update_map.resize(m_world->entityCount() * 2);

			handleHostSending(*host, h);

			if(host->timeout() > 10.0) {
				printf("Disconnecting host (timeout)\n");
				disconnectClient(h);
			}
		}

		for(int h = 0; h < (int)m_clients.size(); h++) {
			ClientInfo &client = m_clients[h];
			if(client.mode == ClientMode::to_be_removed) {
				printf("Client disconnected: %s\n", getRemoteHost(h)->address().toString().c_str());
				removeRemoteHost(h);
				client.mode = ClientMode::invalid;
			}
		}
		
		m_replication_list.clear();

		if(m_current_time >= m_lobby_timeout) {
			OutPacket out(0, -1, -1, PacketInfo::flag_lobby);
			ServerStatusChunk chunk;
			chunk.address = Address();
			//TODO: send map title, which should be shorter
			chunk.map_name = m_config.m_map_name;
			chunk.server_name = m_config.m_server_name;
			chunk.num_players = numActiveClients();
			chunk.max_players = maxPlayers();
			chunk.is_passworded = !m_config.m_password.empty();
			chunk.game_mode = m_game_mode->typeId();
			out << LobbyChunkId::server_status << chunk;
			sendLobbyPacket(out);
			m_lobby_timeout = m_current_time + 10.0;
		}
	}

	void Server::setWorld(PWorld world) {
		DASSERT(world);

		//TODO: different modes
		world->assignGameMode<DeathMatchServer>();
		
		//TODO send level_info to clients
		m_world = world;
		m_world->setReplicator(this);
		m_game_mode = dynamic_cast<GameModeServer*>(m_world->gameMode());

		m_config.m_map_name = m_world? m_world->mapName() : "";
	}

	void Server::replicateEntity(int entity_id) {
		m_replication_list.emplace_back(entity_id);
	}
		
	void Server::sendMessage(net::TempPacket &packet, int target_id) {
		for(int c = 0; c < (int)m_clients.size(); c++) {
			const ClientInfo &client = m_clients[c];
			if(client.isValid() && (target_id == -1 || c == target_id)) {
				RemoteHost *host = getRemoteHost(client.host_id);
				if(host)
					host->enqueChunk(packet.data(), packet.pos(), ChunkType::message, 1);
			}
		}
	}

}

