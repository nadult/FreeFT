// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "net/host.h"
#include "net/base.h"
#include "game/entity.h"
#include "game/world.h"
#include <fwk/bit_vector.h>

namespace net {

	class ServerConfig {
	public:
		ServerConfig();
		ServerConfig(const XMLNode&);
		void save(XMLNode&);

		bool isValid() const;

		bool m_console_mode;
		string m_map_name;
		string m_server_name;
		string m_password;
		int m_port, m_max_players;
	};

	class Server: public net::LocalHost, game::Replicator {
	public:
		Server(const ServerConfig &config);
		~Server();

		enum {
			client_timeout = 10,
		};

		enum class ClientMode {
			invalid,
			connecting,
			connected,
			to_be_removed,
		};

		struct ClientInfo {
			ClientInfo() :mode(ClientMode::invalid), host_id(-1), notify_others(false), is_loading_level(false) { }
			bool isValid() const { return host_id != -1 && mode != ClientMode::invalid; }

			string nick_name;
			ClientMode mode;
			BitVector update_map;
			int host_id;

			bool notify_others;
			bool is_loading_level;
		};

		int numActiveClients() const;
		int maxPlayers() const { return min(m_config.m_max_players, (int)max_remote_hosts); }

		void disconnectClient(int client_id);

		void beginFrame();
		void finishFrame();

		void setWorld(game::PWorld);
		game::PWorld world() { return m_world; }

		const ServerConfig &config() const { return m_config; }

	private:
		void handleHostReceiving(RemoteHost &host, int client_id);
		void handleHostSending(RemoteHost &host, int client_id);
		
		void replicateEntity(int entity_id) override;
		void sendMessage(net::TempPacket&, int target_id) override;

		ServerConfig m_config;
		vector<int> m_replication_list;
		vector<ClientInfo> m_clients; //TODO: change to map?

		game::PWorld m_world;
		game::GameModeServer *m_game_mode;
		double m_current_time;
		double m_lobby_timeout;
	};

}
