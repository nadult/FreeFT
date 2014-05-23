/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "net/host.h"
#include "net/lobby.h"
#include "game/entity.h"
#include "game/world.h"
#include "game/orders.h"

namespace net {

	class Client: public net::LocalHost, game::Replicator {
	public:
		enum class Mode {
			disconnected,
			connecting,
			connected,
		};

		Client(int port = 0);
		~Client();
	
		bool getLobbyData(vector<ServerStatusChunk> &out);
		void requestLobbyData();

		void connect(Address address);
		void disconnect();
		
		Mode mode() const { return m_mode; }

		void beginFrame();
		void finishFrame();

		game::EntityRef actorRef() const { return m_actor_ref; }
		game::PWorld world() { return m_world; }

	protected:
		void entityUpdate(InChunk &chunk);
		void replicateOrder(game::POrder &&order, game::EntityRef entity_ref) override;

	private:
		game::EntityRef m_actor_ref;
		int m_server_id;
		Mode m_mode;
		game::PWorld m_world;

		Address m_server_address;

		vector<game::POrder> m_orders;
		bool m_order_cancels_prev;
		game::OrderTypeId::Type m_order_type;
		double m_order_send_time;
	};

}

#endif
