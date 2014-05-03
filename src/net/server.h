/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "net/host.h"
#include "net/lobby.h"
#include "game/entity.h"
#include "game/world.h"

namespace net {

	class Server: public net::LocalHost, game::Replicator {
	public:
		Server(int port);
		~Server();

		enum {
			max_clients = 32,
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


		game::EntityRef spawnActor(game::EntityRef spawn_zone);
		void disconnectClient(int client_id);

		void beginFrame();
		void finishFrame();

		void createWorld(const string &file_name);
		game::PWorld world() { return m_world; }

		void replicateEntity(int entity_id) override;

	private:
		void handleHostReceiving(RemoteHost &host, Client &client);
		void handleHostSending(RemoteHost &host, Client &client);

		vector<int> m_replication_list;
		vector<Client> m_clients;

		game::PWorld m_world;
		int m_client_count;
		double m_current_time;
		double m_lobby_timeout;
	};

}

#endif
