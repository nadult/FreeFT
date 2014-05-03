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

	ServerLoop::ServerLoop(const string &map_name, int port) {
		Config config = loadConfig("server");
		
		m_server.reset(new net::Server(port));
		m_server->createWorld(map_name);
		m_world = m_server->world();
	
		for(int n = 0; n < m_world->entityCount(); n++) {
			Actor *actor = m_world->refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>();
		}

		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, EntityRef(), config.profiler_enabled));
	}

	bool ServerLoop::tick(double time_diff) {
		m_controller->update();
		m_server->beginFrame();

		m_world->simulate(time_diff);
		m_server->finishFrame();
		m_controller->updateView(time_diff);
		m_controller->draw();

		return !isKeyDown(gfx::Key_esc);
	}

}
