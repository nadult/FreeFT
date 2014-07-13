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
		actor_order,
		class_changed,
		update_client,
		remove_client,
		respawn,

		update_client_info
	);

	class PlayableCharacter: public RefCounter {
	public:
		PlayableCharacter(const Character &character);
		
		void save(Stream&) const;
		void load(Stream&);

		void setCharacterClass(const CharacterClass &char_class) { m_class = char_class; }
		const CharacterClass &characterClass() const { return m_class; }

		void setEntityRef(EntityRef ref) { m_entity_ref = ref; }
		EntityRef entityRef() const { return m_entity_ref; }

		const Character &character() const { return m_character; }
		bool operator==(const PlayableCharacter&) const;

	private:
		Character m_character;
		CharacterClass m_class;
		EntityRef m_entity_ref;
	};

	class GameMode {
	public:
		GameMode(World &world) :m_world(world) { }
		virtual ~GameMode() { }

		GameMode(const GameMode&) = delete;
		void operator=(const GameMode&) = delete;

		virtual GameModeId::Type typeId() const = 0;
		virtual void tick(double time_diff) = 0;
		virtual const vector<PlayableCharacter> playableCharacters() const = 0;

		virtual void onMessage(Stream&, MessageId::Type, int source_id) { }
		virtual bool sendOrder(POrder &&order, EntityRef entity_ref);

	protected:
		EntityRef findSpawnZone(int faction_id) const;
		EntityRef spawnActor(EntityRef spawn_zone, const Proto &proto);
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

	protected:
		void onClientConnected(int client_id, const string &nick_name);
		void onClientDisconnected(int client_id);
		friend class net::Server;

	protected:
		void sendClientInfo(int client_id, int target_id);
		void updateClient(int client_id, const GameClient&);
		void respawn(int client_id, int pc_id, EntityRef spawn_zone);

		std::map<int, GameClient> m_clients;
	};

	class GameModeClient: public GameMode {
	public:
		GameModeClient(World &world, int client_id);

		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;
		const vector<PlayableCharacter> playableCharacters() const override { return m_current.pcs; }

		void setPlayerClassId(int id);
		bool sendOrder(POrder &&order, EntityRef entity_ref) override;

	protected:
		void addPC(const PlayableCharacter&);

		GameClient m_current;
		std::map<int, GameClient> m_others;
		const int m_current_id;
	};

}

#endif

