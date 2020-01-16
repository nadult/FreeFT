// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/death_match.h"
#include "game/world.h"
#include "game/actor.h"
#include "net/base.h"

namespace game {

	DeathMatchServer::ClientInfo::ClientInfo()
		:next_respawn_time(respawn_delay), kills(0), self_kills(0), deaths(0), is_respawning(false) { }

	void DeathMatchServer::ClientInfo::save(MemoryStream &sr) const {
		sr.pack(next_respawn_time, kills, self_kills, deaths, is_respawning);
	}

	void DeathMatchServer::ClientInfo::load(MemoryStream &sr) {
		sr.unpack(next_respawn_time, kills, self_kills, deaths, is_respawning);
	}

	DeathMatchServer::DeathMatchServer(World &world) :GameModeServer(world) {
		attachAIs();
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
					const ActorInventory &inv = pcs[0].characterClass().inventory(true);
					if(respawnPC(PCIndex(client_id, 0), findSpawnZone(0), inv)) {
						replicateClient(client_id);
						info.is_respawning = false;
					}
					else {
						info.next_respawn_time = respawn_delay;
						// TODO: notify client that there was a problem while trying to respawn
					}

					notify_client = true;
				}
			}

			if(notify_client)
				replicateClientInfo(client_id, -1);
		}
	}
		
	void DeathMatchServer::onKill(EntityRef target_ref, EntityRef killer_ref) {
		PCIndex target_idx = findPC(target_ref);
		PCIndex killer_idx = findPC(killer_ref);
		PlayableCharacter *target_pc = pc(target_idx);
		PlayableCharacter *killer_pc = pc(killer_idx);

		bool self_kill = killer_ref == target_ref;
		if(target_pc) {
			m_client_infos[target_idx.client_id].deaths++;
			if(self_kill)
				m_client_infos[target_idx.client_id].self_kills++;
			replicateClientInfo(target_idx.client_id, -1);
		}

		if(target_pc && killer_pc && !self_kill) {
			m_client_infos[killer_idx.client_id].kills++;
			replicateClientInfo(killer_idx.client_id, -1);
		}
	}

	void DeathMatchServer::onMessage(MemoryStream &sr, MessageId msg_type, int source_id) {
		if(msg_type == MessageId::update_client_info) {
		}
		else
			GameModeServer::onMessage(sr, msg_type, source_id);
	}
		
	void DeathMatchServer::replicateClientInfo(int client_id, int target_id) {
		auto temp = memorySaver();
		temp << MessageId::update_client_info;
		encodeInt(temp, client_id);
		temp << m_client_infos[client_id];
		m_world.sendMessage(temp.data(), target_id);
	}
		
	void DeathMatchServer::onClientConnected(int client_id, const string &nick_name) {
		GameModeServer::onClientConnected(client_id, nick_name);
		m_client_infos[client_id] = ClientInfo();
		replicateClientInfo(client_id, -1);
		for(const auto &it : m_client_infos)
			replicateClientInfo(it.first, client_id);
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
	
	void DeathMatchClient::onMessage(MemoryStream &sr, MessageId msg_type, int source_id) {
		if(msg_type == MessageId::update_client_info) {
			ClientInfo new_info;
			int client_id = decodeInt(sr);
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
		
		
	const UserMessage DeathMatchClient::userMessage(UserMessageType type) {
		if(type == UserMessageType::main) {
			if(m_current_info.is_respawning) {
				int time = (int)(m_current_info.next_respawn_time + 0.9999f);
				if(time > 0)
					return UserMessage{format("Respawning in %...", time)};
			}
		}
		else if(type == UserMessageType::log) {

		}

		return UserMessage();
		
	}

}
