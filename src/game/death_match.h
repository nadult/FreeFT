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
			respawn_delay = 5
		};

		//TODO: decrease data sent over the net
		DeathMatchServer(World &world);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;

		struct ClientInfo {
			ClientInfo();
			void save(Stream&) const;
			void load(Stream&);

			double next_respawn_time;
			int kills, deaths;
			bool is_respawning;
		};
		
	private:
		void onClientConnected(int client_id, const string &nick_name) override;
		void onClientDisconnected(int client_id) override;
		void onKill(EntityRef target, EntityRef killer) override;
		void replicateClientInfo(int client_id, int target_id);

		std::map<int, ClientInfo> m_client_infos;
	};

	class DeathMatchClient: public GameModeClient {
	public:
		typedef DeathMatchServer::ClientInfo ClientInfo;

		DeathMatchClient(World &world, int client_id, const string &nick_name);

		GameModeId::Type typeId() const override { return GameModeId::death_match; }
		void tick(double time_diff) override;
		void onMessage(Stream&, MessageId::Type, int source_id) override;
		
		const vector<GameClientStats> stats() const override;

	private:
		ClientInfo m_current_info;
		std::map<int, ClientInfo> m_client_infos;
	};

}

#endif

