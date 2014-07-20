/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/death_match.h"
#include "game/world.h"
#include "game/actor.h"
#include "net/base.h"

namespace game {

	DeathMatchServer::ClientInfo::ClientInfo()
		:next_respawn_time(respawn_delay), kills(0), self_kills(0), deaths(0), is_respawning(false) { }

	void DeathMatchServer::ClientInfo::save(Stream &sr) const {
		sr.pack(next_respawn_time, kills, self_kills, deaths, is_respawning);
	}

	void DeathMatchServer::ClientInfo::load(Stream &sr) {
		sr.unpack(next_respawn_time, kills, self_kills, deaths, is_respawning);
	}

	DeathMatchServer::DeathMatchServer(World &world) :GameModeServer(world) {
		
	}

	void DeathMatchServer::tick(double time_diff) {
		GameModeServer::tick(time_diff);

		for(auto &it :m_clients) {
			int client_id = it.first;
			ClientInfo &info = m_client_infos[client_id];
			vector<PlayableCharacter> &pcs = it.second.pcs;

			EntityRef actor_ref = pcs.empty()? EntityRef() : pcs.front().entityRef();
			const Actor *actor = m_world.refEntity<Actor>(actor_ref);
			bool needs_respawn = (!actor || actor->isDead()) && !pcs.empty();
			bool notify_client = false;

			if(needs_respawn && !info.is_respawning) {
				info.is_respawning = true;
				info.next_respawn_time = respawn_delay + time_diff;
				notify_client = true;
			}

			if(info.is_respawning) {
				info.next_respawn_time -= time_diff;
				if(info.next_respawn_time < 0.0f) {
					respawn(client_id, 0, findSpawnZone(0));
					notify_client = true;
					info.is_respawning = false;
				}
			}

			if(notify_client)
				replicateClientInfo(client_id, -1);
		}
	}
		
	void DeathMatchServer::onKill(EntityRef target_ref, EntityRef killer_ref) {
		auto target = findPC(target_ref);
		auto killer = findPC(killer_ref);

		bool self_kill = killer_ref == target_ref;
		if(target.second) {
			m_client_infos[target.first].deaths++;
			if(self_kill)
				m_client_infos[target.first].self_kills++;
			replicateClientInfo(target.first, -1);
		}

		if(target.second && killer.second && !self_kill) {
			m_client_infos[killer.first].kills++;
			replicateClientInfo(killer.first, -1);
		}
	}

	void DeathMatchServer::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {
		if(msg_type == MessageId::update_client_info) {
		}
		else
			GameModeServer::onMessage(sr, msg_type, source_id);
	}
		
	void DeathMatchServer::replicateClientInfo(int client_id, int target_id) {
		net::TempPacket chunk;
		chunk << MessageId::update_client_info;
		chunk.encodeInt(client_id);
		chunk << m_client_infos[client_id];
		m_world.sendMessage(chunk, target_id);
	}
		
	void DeathMatchServer::onClientConnected(int client_id, const string &nick_name) {
		GameModeServer::onClientConnected(client_id, nick_name);
		m_client_infos[client_id] = ClientInfo();
	}

	void DeathMatchServer::onClientDisconnected(int client_id) {
		GameModeServer::onClientDisconnected(client_id);
		m_client_infos.erase(client_id);
	}


	DeathMatchClient::DeathMatchClient(World &world, int client_id, const string &nick_name)
		:GameModeClient(world, client_id, nick_name) {
	}
		
	void DeathMatchClient::tick(double time_diff) {
		GameModeClient::tick(time_diff);
	
		if(m_current_info.is_respawning)
			m_current_info.next_respawn_time -= time_diff;
	}
	
	void DeathMatchClient::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {
		if(msg_type == MessageId::update_client_info) {
			ClientInfo new_info;
			int client_id = sr.decodeInt();
			sr >> new_info;
			m_client_infos[client_id] = new_info;
			if(client_id == m_current_id)
				m_current_info = new_info;
		}
		else
			GameModeClient::onMessage(sr, msg_type, source_id);
	}
		
	void DeathMatchClient::onClientDisconnected(int client_id) {
		GameModeClient::onClientDisconnected(client_id);
		m_client_infos.erase(client_id);
	}
		
	const vector<GameClientStats> DeathMatchClient::stats() const {
		vector<GameClientStats> out;
		for(const auto &info : m_client_infos) {
			auto client_it = m_clients.find(info.first);
			string nick_name = client_it == m_clients.end()? string() : client_it->second.nick_name;
			out.emplace_back(GameClientStats{nick_name, info.first, info.second.kills, info.second.deaths});
		}
		return out;
	}
		
		
	const UserMessage DeathMatchClient::userMessage(UserMessageType::Type type) {
		if(type == UserMessageType::main) {
			if(m_current_info.is_respawning) {
				int time = (int)(m_current_info.next_respawn_time + 0.9999f);
				if(time > 0)
					return UserMessage{format("Respawning in %d...", time)};
			}
		}
		else if(type == UserMessageType::log) {

		}

		return UserMessage();
		
	}

}
