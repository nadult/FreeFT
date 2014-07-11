/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/death_match.h"
#include "game/world.h"
#include "game/actor.h"
#include "net/base.h"

namespace game {

	void DeathMatchServer::ClientInfo::save(Stream &sr) const {
		sr.pack(next_respawn_time, kills, deaths, is_dead, is_respawning);
	}

	void DeathMatchServer::ClientInfo::load(Stream &sr) {
		sr.unpack(next_respawn_time, kills, deaths, is_dead, is_respawning);
	}

	DeathMatchServer::DeathMatchServer(World &world) :GameModeServer(world) {
		
	}

	void DeathMatchServer::tick(double time_diff) {
		GameModeServer::tick(time_diff);

		for(auto it = m_clients.begin(); it != m_clients.end(); ++it) {
			int id = it->first;
			if(id >= (int)m_client_infos.size())
				m_client_infos.resize(id + 1);
			ClientInfo &info = m_client_infos[id];
			vector<PlayableCharacter> &pcs = it->second.pcs;

			EntityRef actor_ref = pcs.empty()? EntityRef() : pcs.front().entityRef();
			const Actor *actor = m_world.refEntity<Actor>(actor_ref);
			bool needs_respawn = (!actor || actor->isDead()) && !pcs.empty();
			bool notify_client = false;

			if(actor && actor->isDead() && !info.is_dead) {
				info.is_dead = true;
				info.deaths++;
				notify_client = true;
			}

			if(needs_respawn && !info.is_respawning) {
				info.is_respawning = true;
				info.next_respawn_time = respawn_delay + time_diff;
				notify_client = true;
			}

			if(info.is_respawning) {
				info.next_respawn_time -= time_diff;
				if(info.next_respawn_time < 0.0f) {
					respawn(id, 0, findSpawnZone(0));
					notify_client = true;
				}
			}

			if(notify_client) {
				net::TempPacket chunk;
				chunk << MessageId::update_client_info;
				chunk << info;
				m_world.sendMessage(chunk, id);
			}
		}
	}

	void DeathMatchServer::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {
		if(msg_type == MessageId::update_client_info) {
		}
		else
			GameModeServer::onMessage(sr, msg_type, source_id);
	}


	DeathMatchClient::DeathMatchClient(World &world, int client_id) :GameModeClient(world, client_id) {
	}

	void DeathMatchClient::tick(double time_diff) {
		GameModeClient::tick(time_diff);
	}
	
	void DeathMatchClient::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {

	}


}
