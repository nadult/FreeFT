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
				actor->attachAI<SimpleAI>();
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

		PEntity actor = (PEntity)new Actor(proto, inv);

		FBox spawn_box = spawn_zone->boundingBox();
		float3 spawn_pos = spawn_box.center();

		actor->setPos(spawn_pos);
		while(!m_world.findAny(actor->boundingBox(), {Flags::all | Flags::colliding})) {
			spawn_pos.y -= 1.0f;
			actor->setPos(spawn_pos);
		}

		spawn_pos.y += 1.0f;
		actor->setPos(spawn_pos);

		return m_world.addEntity(std::move(actor));
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
			updateClient(source_id, new_client);
		}
	}

	void GameModeServer::sendClientInfo(int client_id, int target_id) {
		net::TempPacket chunk;
		chunk << MessageId::update_client;
		chunk.encodeInt(client_id);
		chunk << m_clients[client_id];
		m_world.sendMessage(chunk, target_id);
	}
		
	void GameModeServer::updateClient(int client_id, const GameClient &new_client) {
		bool is_new = m_clients.find(client_id) == m_clients.end();

		m_clients[client_id] = new_client;
		for(auto it = m_clients.begin(); it != m_clients.end(); ++it) {
			sendClientInfo(client_id, it->first);
			if(!is_new)
				sendClientInfo(it->first, client_id);
		}
	}
	
	void GameModeServer::onClientConnected(int client_id, const string &nick_name) {
		GameClient new_client;
		new_client.nick_name = nick_name;
		updateClient(client_id, new_client);
	}
		
	void GameModeServer::onClientDisconnected(int client_id) {
		auto del_it = m_clients.find(client_id);
		if(del_it != m_clients.end()) {
			for(auto &pc : del_it->second.pcs)
				m_world.removeEntity(pc.entityRef());
			m_clients.erase(del_it);

			for(auto it = m_clients.begin(); it != m_clients.end(); ++it) {
				net::TempPacket chunk;
				chunk << MessageId::remove_client;
				chunk.encodeInt(client_id);
				m_world.sendMessage(chunk, it->first);
			}
		}
	}
		
	void GameModeServer::respawn(int client_id, int pc_id, EntityRef spawn_zone) {
		DASSERT(client_id >= 0 && client_id < (int)m_clients.size());
		DASSERT(pc_id >= 0 && pc_id < (int)m_clients[client_id].pcs.size());

		GameClient client = m_clients[client_id];
		PlayableCharacter &pc = client.pcs[pc_id];
		if(pc.entityRef())
			m_world.removeEntity(pc.entityRef());
		EntityRef actor_ref = spawnActor(spawn_zone, pc.character().proto(), pc.characterClass().inventory(false));
		Actor *actor = m_world.refEntity<Actor>(actor_ref);
		DASSERT(actor);
		actor->setClientId(client_id);
		pc.setEntityRef(actor_ref);
		updateClient(client_id, client);
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
			m_others[new_id] = new_client;

			if(new_id == m_current_id)
				m_current = new_client;
		}
		else if(msg_type == MessageId::remove_client) {
			int remove_id = sr.decodeInt();
			auto it = m_others.find(remove_id);
			if(it != m_others.end())
				m_others.erase(it);
		}
	}
	
	bool GameModeClient::sendOrder(POrder &&order, EntityRef entity_ref) {
		net::TempPacket chunk;
		chunk << MessageId::actor_order;
		chunk << entity_ref << order;
		m_world.sendMessage(chunk);
		return true;
	}
		
	bool GameModeClient::addPC(const Character &character) {
		if((int)m_current.pcs.size() >= m_max_pcs)
			return false;

		for(const auto &pc : m_current.pcs)
			if(pc.character().name() == character.name())
				return false;

		PlayableCharacter new_char(character);
		m_current.pcs.push_back(new_char);	
		net::TempPacket chunk;
		chunk << MessageId::update_client;
		chunk << m_current;
		m_world.sendMessage(chunk);
		return true;
	}
		
	bool GameModeClient::setPCClass(const Character &character, const CharacterClass &char_class) {
		for(auto &pc : m_current.pcs)
			if(pc.character() == character) {
				pc.setCharacterClass(char_class);
				net::TempPacket chunk;
				chunk << MessageId::update_client;
				chunk << m_current;
				m_world.sendMessage(chunk);
				return true;
			}

		return false;
	}

}
