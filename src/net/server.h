/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "net/host.h"
#include "net/chunks.h"
#include "game/entity.h"
#include "game/world.h"

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

		struct Client {
			Client() :mode(ClientMode::invalid), host_id(-1) { }

			bool isValid() const { return host_id != -1 && mode != ClientMode::invalid; }

			ClientMode mode;
			BitVector update_map;
			game::EntityRef actor_ref;
			int host_id;
		};

		int maxPlayers() const { return min(m_config.m_max_players, (int)max_remote_hosts); }
		game::EntityRef spawnActor(game::EntityRef spawn_zone);
		void disconnectClient(int client_id);

		void beginFrame();
		void finishFrame();

		game::PWorld world() { return m_world; }

		void replicateEntity(int entity_id) override;

		const ServerConfig &config() const { return m_config; }

	private:
		void handleHostReceiving(RemoteHost &host, Client &client);
		void handleHostSending(RemoteHost &host, Client &client);

		ServerConfig m_config;
		vector<int> m_replication_list;
		vector<Client> m_clients;

		game::PWorld m_world;
		int m_client_count;
		double m_current_time;
		double m_lobby_timeout;
	};

}

#endif
