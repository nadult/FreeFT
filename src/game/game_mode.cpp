/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/game_mode.h"
#include "game/world.h"
#include "game/actor.h"
#include "game/turret.h"
#include "game/inventory.h"
#include "game/trigger.h"
#include "game/brain.h"
#include "net/base.h"
#include "game/tile.h"

namespace game {
		
	void GameClient::save(Stream &sr) const {
		sr << nick_name;
		encodeInt(sr, (int)pcs.size());
		for(const auto &pc : pcs)
			sr << pc;
	}

	void GameClient::load(Stream &sr) {
		sr >> nick_name;
		int count = decodeInt(sr);
		pcs.clear();
		for(int n = 0; n < count; n++) 
			pcs.emplace_back(PlayableCharacter(sr));
	}


	GameMode::GameMode(World &world, int current_id)
			:m_world(world), m_current_id(current_id) {
		if(!m_world.isClient())
			initAISpawnZones();
	}
		
	GameMode::~GameMode() { }

	bool GameMode::sendOrder(POrder &&order, EntityRef entity_ref) {
		if( ThinkingEntity *entity = m_world.refEntity<ThinkingEntity>(entity_ref) )
			return entity->setOrder(std::move(order));
		return false;
	}
		
	void GameMode::attachAIs() {
		for(int n = 0; n < m_world.entityCount(); n++) {
			Actor *actor = m_world.refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<ActorBrain>(&m_world);
			Turret *turret = m_world.refEntity<Turret>(n);
			if(turret) {
				turret->attachAI<ActorBrain>(&m_world);
				ActorBrain *brain = static_cast<ActorBrain*>(turret->AI());
				brain->setEnemyFactions({1, 2});
			}
		}
	}

	void GameMode::tick(double time_diff) {
		if(!m_world.isClient())
			spawnAIs(time_diff);
	}
		
	void GameMode::initAISpawnZones() {
		for(int n = 0; n < m_world.entityCount(); n++) {
			Trigger *trigger = m_world.refEntity<Trigger>(n);
			if(trigger && trigger->classId() == TriggerClassId::spawn_zone && trigger->factionId() != 0) {
				AISpawnZone spawn;
				spawn.ref = trigger->ref();
				spawn.next_spawn_time = 0.0;
				spawn.entities.resize(trigger->spawnLimit());
				m_spawn_zones.emplace_back(spawn);
			}
		}
	}

	void GameMode::spawnAIs(double time_diff) {
		for(auto &spawn : m_spawn_zones) {
			const Trigger *spawn_zone = m_world.refEntity<Trigger>(spawn.ref);
			if(!spawn_zone)
				continue;

			spawn.next_spawn_time -= time_diff;
			if(spawn.next_spawn_time <= 0.0) {
				int idx = -1;

				for(int n = 0; n < (int)spawn.entities.size(); n++) {
					const Actor *ai = m_world.refEntity<Actor>(spawn.entities[n]);
					bool is_dead = ai && ai->isDead();

					if( (idx == -1 && is_dead) || !ai )
						idx = n;
				}

				if(idx != -1) {
					m_world.removeEntity(spawn.entities[idx]);

					EntityRef ai = spawnActor(spawn.ref, getProto("rad_scorpion", ProtoId::actor), ActorInventory());
					spawn.entities[idx] = ai;
					if(ai) {
						Actor *actor = m_world.refEntity<Actor>(ai);
						DASSERT(actor);
						actor->attachAI<ActorBrain>(&m_world);
					}
				}

				spawn.next_spawn_time = spawn_zone->spawnDelay();
			}
		}

	}
	
	game::EntityRef GameMode::findSpawnZone(int faction_id) const {
		Trigger *spawn_zone = nullptr;

		vector<Trigger*> spawn_zones;
		for(int n = 0; n < m_world.entityCount(); n++) {
			Trigger *trigger = m_world.refEntity<Trigger>(n);
			if(trigger && trigger->classId() == TriggerClassId::spawn_zone && trigger->factionId() == faction_id)
				spawn_zones.push_back(trigger);
		}

		return spawn_zones.empty()? EntityRef() : spawn_zones[rand() % spawn_zones.size()]->ref();
	}

	EntityRef GameMode::spawnActor(EntityRef spawn_zone_ref, const Proto &proto, const ActorInventory &inv) {
		const Trigger *spawn_zone = m_world.refEntity<Trigger>(spawn_zone_ref);
		DASSERT(spawn_zone);

		ActorInventory temp = inv;
		PEntity entity = (PEntity)new Actor(proto, temp);

		FBox spawn_box = spawn_zone->boundingBox();
		float3 bbox_size = entity->bboxSize();
		spawn_box.max.x -= bbox_size.x;
		spawn_box.max.z -= bbox_size.z;

		float3 spawn_pos;
		int it = 0, it_max = 100;
		for(; it < it_max; it++) {
			spawn_pos = spawn_box.min + float3(frand() * spawn_box.width(), 1.0f, frand() * spawn_box.depth());
			FBox bbox(spawn_pos, spawn_pos + bbox_size);
			ObjectRef isect = m_world.findAny(bbox, {Flags::all | Flags::colliding});

			if(isect.isEmpty()) {
				spawn_pos.y -= 1.0f;
				break;
			}
		}
		if(it == it_max)
			return EntityRef();
		
		entity->setPos(spawn_pos);
		EntityRef out = m_world.addEntity(std::move(entity));
		Actor *actor = m_world.refEntity<Actor>(out);
		actor->fixPosition();
		actor->setFactionId(spawn_zone->factionId());
		return out;
	}

	bool GameMode::respawnPC(const PCIndex &index, EntityRef spawn_zone, const ActorInventory &inv) {
		DASSERT(isValidIndex(index));

		PlayableCharacter *pc = this->pc(index);
		if(pc->entityRef())
			m_world.removeEntity(pc->entityRef());

		EntityRef actor_ref = spawnActor(spawn_zone, pc->character().proto(), inv);
		if(!actor_ref)
			return false;

		pc->setEntityRef(actor_ref);
		Actor *actor = m_world.refEntity<Actor>(actor_ref);
		DASSERT(actor);
		actor->setClientId(index.client_id);
		return true;
	}
		
	GameClient *GameMode::client(int client_id) {
		auto it = m_clients.find(client_id);
		return it == m_clients.end()? nullptr : &it->second;
	}

	PlayableCharacter *GameMode::pc(const PCIndex &index) {
		if(!isValidIndex(index))
			return nullptr;
		return &m_clients[index.client_id].pcs[index.pc_id];
	}

	bool GameMode::isValidIndex(const PCIndex &index) const {
		const GameClient *client = this->client(index.client_id);
		return client && index.pc_id >= 0 && index.pc_id < (int)client->pcs.size();
	}
		
	const PCIndex GameMode::findPC(EntityRef ref) const {
		if( const Actor *actor = m_world.refEntity<Actor>(ref) )
			if( const GameClient *client = this->client(actor->clientId()) )
				for(int n = 0; n < (int)client->pcs.size(); n++)
					if(client->pcs[n].entityRef() == ref)
						return PCIndex(actor->clientId(), n);
		return PCIndex();
	}
	
	
	GameModeServer::GameModeServer(World &world) :GameMode(world, -1) {
		ASSERT(world.isServer());
	}

	void GameModeServer::tick(double time_diff) {
		GameMode::tick(time_diff);
	}

	void GameModeServer::onMessage(Stream &sr, MessageId msg_type, int source_id) {
		if(msg_type == MessageId::actor_order) {
			EntityRef actor_ref;
			POrder order;
			sr >> actor_ref;
			order.reset(Order::construct(sr));

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
			sr >> m_clients[source_id];
			//TODO: verification?
			replicateClient(source_id);
		}
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

	GameModeClient::GameModeClient(World &world, int client_id, const string &nick_name)
		:GameMode(world, client_id) {
		m_current.nick_name = nick_name;
		ASSERT(world.isClient());
		m_max_pcs = 1;
	}
	
	void GameModeClient::tick(double time_diff) {
		GameMode::tick(time_diff);
	}

	void GameModeClient::onMessage(Stream &sr, MessageId msg_type, int source_id) {
		if(msg_type == MessageId::update_client) {
			GameClient new_client;
			int new_id = decodeInt(sr);
			sr >> new_client;
			m_clients[new_id] = new_client;

			if(new_id == m_current_id)
				m_current = new_client;
		}
		else if(msg_type == MessageId::remove_client) {
			onClientDisconnected(decodeInt(sr));
		}
	}
	
	void GameModeClient::onClientDisconnected(int client_id) {
		m_clients.erase(client_id);
	}
	
	bool GameModeClient::sendOrder(POrder &&order, EntityRef entity_ref) {
		if(!order || !entity_ref)
			return false;

		net::TempPacket chunk;
		chunk << MessageId::actor_order;
		chunk << entity_ref << order->typeId() << *order;
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
