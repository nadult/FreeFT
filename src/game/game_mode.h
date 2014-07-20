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

	DECLARE_ENUM(MessageId,
		sound,

		actor_order,
		class_changed,
		update_client,
		remove_client,
		respawn,

		update_client_info
	);

	DECLARE_ENUM(UserMessageType,
		main,
		log
	);

	struct UserMessage {
		UserMessage(const string &text) :text(text) { }
		UserMessage() { }

		string text;
	};

	class GameMode {
	public:
		GameMode(World &world) :m_world(world) { }
		virtual ~GameMode() { }

		GameMode(const GameMode&) = delete;
		void operator=(const GameMode&) = delete;

		virtual GameModeId::Type typeId() const = 0;
		virtual const vector<PlayableCharacter> playableCharacters() const = 0;

		virtual void onKill(EntityRef target, EntityRef killer) { }
		virtual const UserMessage userMessage(UserMessageType::Type) { return UserMessage(); }

	protected:
		virtual void tick(double time_diff) = 0;
		virtual void onMessage(Stream&, MessageId::Type, int source_id) { }
		virtual bool sendOrder(POrder &&order, EntityRef entity_ref);

		friend class World;

	protected:
		EntityRef findSpawnZone(int faction_id) const;
		EntityRef spawnActor(EntityRef spawn_zone, const Proto &proto, const ActorInventory&);
		void attachAIs();

		World &m_world;
	};

	struct GameClient {
		void save(Stream&) const;
		void load(Stream&);

		string nick_name;
		vector<PlayableCharacter> pcs;
	};

	class GameModeServer: public GameMode {
	public:
		GameModeServer(World &world);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;

		const vector<PlayableCharacter> playableCharacters() const override { return {}; };
		void respawn(int client_id, int pc_id, EntityRef spawn_zone);
		pair<int, PlayableCharacter*> findPC(EntityRef);

	protected:
		virtual void onClientConnected(int client_id, const string &nick_name);
		virtual void onClientDisconnected(int client_id);
		friend class net::Server;
		
		void replicateClient(int client_id, int target_id = -1);

		std::map<int, GameClient> m_clients;
	};

	struct GameClientStats {
		string nick_name;
		int client_id;
		int kills, deaths;
	};

	class GameModeClient: public GameMode {
	public:
		GameModeClient(World &world, int client_id, const string &nick_name);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;
		const vector<PlayableCharacter> playableCharacters() const override { return m_current.pcs; }

		virtual const vector<GameClientStats> stats() const { return {}; }

		const string &currentNickName() const { return m_current.nick_name; }
		bool addPC(const PlayableCharacter&);
		bool setPCClassId(const Character&, int class_id);

		bool sendOrder(POrder &&order, EntityRef entity_ref) override;

	protected:
		GameClient m_current;
		std::map<int, GameClient> m_others;
		const int m_current_id;
		int m_max_pcs;
	};

}

#endif

