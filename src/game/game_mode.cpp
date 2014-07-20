/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/game_mode.h"
#include "game/world.h"
#include "game/actor.h"
#include "game/inventory.h"
#include "game/trigger.h"
#include "net/socket.h"

namespace game {
		
	bool GameMode::sendOrder(POrder &&order, EntityRef entity_ref) {
		if( Actor *entity = m_world.refEntity<Actor>(entity_ref) )
			return entity->setOrder(std::move(order));
		return false;
	}
		
	void GameMode::attachAIs() {
		for(int n = 0; n < m_world.entityCount(); n++) {
			Actor *actor = m_world.refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>(PWorld(&m_world));
		}
	}
	
	game::EntityRef GameMode::findSpawnZone(int faction_id) const {
		Trigger *spawn_zone = nullptr;

		vector<Trigger*> spawn_zones;
		for(int n = 0; n < m_world.entityCount(); n++) {
			Trigger *trigger = m_world.refEntity<Trigger>(n);
			if(trigger && trigger->classId() == TriggerClassId::spawn_zone)
				spawn_zones.push_back(trigger);
		}

		return spawn_zones.empty()? EntityRef() : spawn_zones[rand() % spawn_zones.size()]->ref();
	}

	EntityRef GameMode::spawnActor(EntityRef spawn_zone_ref, const Proto &proto, const ActorInventory &inv) {
		const Trigger *spawn_zone = m_world.refEntity<Trigger>(spawn_zone_ref);
		DASSERT(spawn_zone);

		ActorInventory temp = inv;
		PEntity actor = (PEntity)new Actor(proto, temp);

		FBox spawn_box = spawn_zone->boundingBox();
		float3 bbox_size = actor->bboxSize();
		spawn_box.max.x -= bbox_size.x;
		spawn_box.max.z -= bbox_size.z;

		float3 spawn_pos;
		for(int it = 0; it < 100; it++) {
			spawn_pos = spawn_box.min + float3(frand() * spawn_box.width(), 1.0f, frand() * spawn_box.depth());
			if(!m_world.findAny(actor->boundingBox(), {Flags::all | Flags::colliding})) {
				spawn_pos.y -= 1.0f;
				break;
			}
		}
		
		actor->setPos(spawn_pos);
		EntityRef out = m_world.addEntity(std::move(actor));
		m_world.refEntity<Actor>(out)->fixPosition();
		return out;
	}
	
	void GameClient::save(Stream &sr) const {
		sr << nick_name;
		sr.encodeInt((int)pcs.size());
		for(int n = 0; n < (int)pcs.size(); n++)
			sr << pcs[n];
	}

	void GameClient::load(Stream &sr) {
		sr >> nick_name;
		int count = sr.decodeInt();
		pcs.clear();
		pcs.reserve(count);
		for(int n = 0; n < count; n++) 
			pcs.emplace_back(PlayableCharacter(sr));
	}
		
	GameModeServer::GameModeServer(World &world) :GameMode(world) {
		ASSERT(world.isServer());
	}

	void GameModeServer::tick(double time_diff) {
	}

	void GameModeServer::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {
		if(msg_type == MessageId::actor_order) {
			EntityRef actor_ref;
			POrder order;
			sr >> actor_ref >> order;

			auto it = m_clients.find(source_id);
			if(it != m_clients.end() && order) {
				for(int n = 0; n < (int)it->second.pcs.size(); n++)
					if(it->second.pcs[n].entityRef() == actor_ref) {
						sendOrder(std::move(order), actor_ref);
						break;
					}
			}
		}
		else if(msg_type == MessageId::update_client) {
			GameClient new_client;
			sr >> new_client;
			//TODO: verification?
			m_clients[source_id] = new_client;
			replicateClient(source_id);
		}
	}
		
	pair<int, PlayableCharacter*> GameModeServer::findPC(EntityRef ref) {
		const Actor *actor = m_world.refEntity<Actor>(ref);
		if(actor) {
			auto it = m_clients.find(actor->clientId());
			if(it != m_clients.end())
				for(auto &pc : it->second.pcs)
					if(pc.entityRef() == ref)
						return make_pair(actor->clientId(), &pc);
		}

		return make_pair(-1, nullptr);
	}

	void GameModeServer::replicateClient(int client_id, int target_id) {
		net::TempPacket chunk;
		chunk << MessageId::update_client;
		chunk.encodeInt(client_id);
		chunk << m_clients[client_id];
		m_world.sendMessage(chunk, target_id);
	}
	
	void GameModeServer::onClientConnected(int client_id, const string &nick_name) {
		GameClient new_client;
		new_client.nick_name = nick_name;
		m_clients[client_id] = new_client;
		replicateClient(client_id, -1);
		for(auto &it : m_clients)
			replicateClient(it.first, client_id);
	}
		
	void GameModeServer::onClientDisconnected(int client_id) {
		auto del_it = m_clients.find(client_id);
		if(del_it != m_clients.end()) {
			for(auto &pc : del_it->second.pcs)
				m_world.removeEntity(pc.entityRef());
			m_clients.erase(del_it);
				
			net::TempPacket chunk;
			chunk << MessageId::remove_client;
			chunk.encodeInt(client_id);
			m_world.sendMessage(chunk, -1);
		}
	}
		
	void GameModeServer::respawn(int client_id, int pc_id, EntityRef spawn_zone) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		DASSERT(pc_id >= 0 && pc_id < (int)m_clients[client_id].pcs.size());

		GameClient &client = m_clients[client_id];
		PlayableCharacter &pc = client.pcs[pc_id];
		if(pc.entityRef())
			m_world.removeEntity(pc.entityRef());
		EntityRef actor_ref = spawnActor(spawn_zone, pc.character().proto(), pc.characterClass().inventory(true));
		Actor *actor = m_world.refEntity<Actor>(actor_ref);
		DASSERT(actor);
		actor->setClientId(client_id);
		pc.setEntityRef(actor_ref);
		replicateClient(client_id, -1);
	}

	GameModeClient::GameModeClient(World &world, int client_id, const string &nick_name)
		:GameMode(world), m_current_id(client_id) {
		m_current.nick_name = nick_name;
		ASSERT(world.isClient());
		m_max_pcs = 1;
	}
	
	void GameModeClient::tick(double time_diff) {

	}

	void GameModeClient::onMessage(Stream &sr, MessageId::Type msg_type, int source_id) {
		if(msg_type == MessageId::update_client) {
			GameClient new_client;
			int new_id = sr.decodeInt();
			sr >> new_client;
			m_clients[new_id] = new_client;

			if(new_id == m_current_id)
				m_current = new_client;
		}
		else if(msg_type == MessageId::remove_client) {
			onClientDisconnected(sr.decodeInt());
		}
	}
	
	void GameModeClient::onClientDisconnected(int client_id) {
		m_clients.erase(client_id);
	}
	
	bool GameModeClient::sendOrder(POrder &&order, EntityRef entity_ref) {
		net::TempPacket chunk;
		chunk << MessageId::actor_order;
		chunk << entity_ref << order;
		m_world.sendMessage(chunk);
		return true;
	}
		
	bool GameModeClient::addPC(const PlayableCharacter &new_char) {
		if((int)m_current.pcs.size() >= m_max_pcs)
			return false;

		for(const auto &pc : m_current.pcs)
			if(pc.character().name() == new_char.character().name())
				return false;

		m_current.pcs.push_back(new_char);	
		net::TempPacket chunk;
		chunk << MessageId::update_client;
		chunk << m_current;
		m_world.sendMessage(chunk);
		return true;
	}
		
	bool GameModeClient::setPCClassId(const Character &character, int class_id) {
		for(auto &pc : m_current.pcs)
			if(pc.character() == character) {
				pc.setClassId(class_id);
				net::TempPacket chunk;
				chunk << MessageId::update_client;
				chunk << m_current;
				m_world.sendMessage(chunk);
				return true;
			}

		return false;
	}

}
