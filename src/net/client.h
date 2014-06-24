/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "net/host.h"
#include "net/chunks.h"
#include "game/entity.h"
#include "game/world.h"
#include "game/orders.h"

namespace net {

	class Client: public net::LocalHost, public game::Replicator {
	public:
		enum class Mode {
			disconnected,
			connecting,
			refused,
			timeout,

			waiting_for_world_update,
			world_updated,
			playing,
		};

		enum { timeout = 5 };

		Client(int port = 0);
		~Client();
	
		bool getLobbyData(vector<ServerStatusChunk> &out);
		void requestLobbyData();

		void connect(Address address);
		void disconnect();

		int clientId() const { return m_client_id; }
		
		Mode mode() const { return m_mode; }

		void beginFrame();
		void finishFrame();

		const LevelInfoChunk &levelInfo() const { return m_level_info; }
		void setWorld(game::PWorld);
		bool needWorldUpdate() const { return m_mode == Mode::waiting_for_world_update; }

		game::EntityRef actorRef() const { return m_level_info.actor_ref; }
		game::PWorld world() { return m_world; }

	protected:
		void entityUpdate(InChunk &chunk);
		void replicateOrder(game::POrder &&order, game::EntityRef entity_ref) override;
		void sendMessage(net::TempPacket&, int target_id) override;

	private:
		LevelInfoChunk m_level_info;
		game::PWorld m_world;

		Address m_server_address;
		int m_server_id;
		int m_client_id;
		Mode m_mode;

		vector<game::POrder> m_orders;
	};

}

#endif
