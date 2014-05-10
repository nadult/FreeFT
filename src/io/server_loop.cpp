/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/server_loop.h"
#include "net/server.h"
#include "gfx/device.h"
#include "sys/config.h"
#include "game/actor.h"
#include "game/world.h"

using namespace game;

namespace io {

	net::PServer createServer(const string &map_name, int port) {
		net::PServer server(new net::Server(port));
		server->createWorld(map_name);
		return server;
	}

	ServerLoop::ServerLoop(net::PServer server) {
		DASSERT(server && server->world());
		m_server = std::move(server);
		m_world = server->world();
		Config config = loadConfig("server");
		
		for(int n = 0; n < m_world->entityCount(); n++) {
			Actor *actor = m_world->refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>();
		}

		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, EntityRef(), config.profiler_enabled));
	}

	bool ServerLoop::tick(double time_diff) {
		m_controller->update(time_diff);
		m_server->beginFrame();

		m_world->simulate(time_diff);
		m_server->finishFrame();
		m_controller->updateView(time_diff);
		m_controller->draw();

		return !isKeyDown(gfx::Key_esc);
	}

}
