/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_DEATH_MATCH_H
#define GAME_DEATH_MATCH_H

#include "game/game_mode.h"

namespace game {

	class DeathMatchServer: public GameModeServer {
	public:
		enum {
			respawn_delay = 10
		};

		DeathMatchServer(World &world);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;

		struct ClientInfo {
			ClientInfo() :next_respawn_time(respawn_delay), kills(0), deaths(0), is_respawning(false), is_dead(false) { }
			void save(Stream&) const;
			void load(Stream&);

			double next_respawn_time;
			int kills, deaths;
			bool is_dead, is_respawning;
		};
		
	private:
		vector<ClientInfo> m_client_infos;
	};

	class DeathMatchClient: public GameModeClient {
	public:
		DeathMatchClient(World &world, int client_id);

		void addPlayableCharacter(const Character&);
		void setCharacterClass(const CharacterClass&);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;

		typedef DeathMatchServer::ClientInfo ClientInfo;

	private:
		ClientInfo m_current_info;
	};

}

#endif

