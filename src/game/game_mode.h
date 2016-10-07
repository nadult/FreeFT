/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_GAME_MODE_H
#define GAME_GAME_MODE_H

#include "game/base.h"
#include "game/inventory.h"
#include "game/character.h"
#include "game/orders.h"
#include "game/character.h"

namespace game {

	DEFINE_ENUM(UserMessageType,
		main,
		log
	);

	struct UserMessage {
		UserMessage(const string &text) :text(text) { }
		UserMessage() { }

		string text;
	};

	struct GameClient {
		void save(Stream&) const;
		void load(Stream&);

		string nick_name;
		vector<PlayableCharacter> pcs;
	};

	struct PCIndex {
		PCIndex(int client_id, int pc_id)
			:client_id(client_id), pc_id(pc_id) { }
		PCIndex() :client_id(-1), pc_id(-1) { }

		int client_id;
		int pc_id;
	};

	class GameMode {
	public:
		GameMode(World &world, int current_id);
		virtual ~GameMode();

		GameMode(const GameMode&) = delete;
		void operator=(const GameMode&) = delete;

		virtual GameModeId typeId() const = 0;

		virtual void onKill(EntityRef target, EntityRef killer) { }
	
		const GameClient *currentClient() const { return client(m_current_id); }
		const GameClient *client(int client_id) const { return const_cast<GameMode*>(this)->client(client_id); }
		const PlayableCharacter *pc(const PCIndex &index) const { return const_cast<GameMode*>(this)->pc(index); }
		bool isValidIndex(const PCIndex &index) const;
		const PCIndex findPC(EntityRef) const;

		GameClient *client(int client_id);
		PlayableCharacter *pc(const PCIndex&);

		virtual void tick(double time_diff);
		virtual void onMessage(Stream&, MessageId, int source_id) { }
		virtual bool sendOrder(POrder &&order, EntityRef entity_ref);

		virtual const UserMessage userMessage(UserMessageType) { return UserMessage(); }

		struct AISpawnZone {
			EntityRef ref;
			double next_spawn_time;
			vector<EntityRef> entities;
		};

		void initAISpawnZones();
		void spawnAIs(double time_diff);

	protected:
		EntityRef findSpawnZone(int faction_id) const;
		EntityRef spawnActor(EntityRef spawn_zone, const Proto &proto, const ActorInventory&);
		bool respawnPC(const PCIndex &index, EntityRef spawn_zone, const ActorInventory&);
		void attachAIs();

		World &m_world;
		std::map<int, GameClient> m_clients;
		vector<AISpawnZone> m_spawn_zones;
		const int m_current_id;
	};

	class GameModeServer: public GameMode {
	public:
		GameModeServer(World &world);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId, int source_id) override;

	protected:
		virtual void onClientConnected(int client_id, const string &nick_name);
		virtual void onClientDisconnected(int client_id);
		friend class net::Server;
		
		void replicateClient(int client_id, int target_id = -1);
	};

	struct GameClientStats {
		string nick_name;
		int client_id;
		int kills, deaths;
	};

	//TODO: handle disconnect / kick
	class GameModeClient: public GameMode {
	public:
		GameModeClient(World &world, int client_id, const string &nick_name);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId, int source_id) override;
		virtual const vector<GameClientStats> stats() const { return {}; }

		const string &currentNickName() const { return m_current.nick_name; }
		bool addPC(const PlayableCharacter&);
		bool setPCClassId(const Character&, int class_id);

		bool sendOrder(POrder &&order, EntityRef entity_ref) override;

	protected:
		virtual void onClientDisconnected(int client_id);

		GameClient m_current;
		int m_max_pcs;
	};

}

#endif

