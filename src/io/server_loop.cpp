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

	ServerLoop::ServerLoop(net::PServer &&server) {
		DASSERT(server && server->world());
		m_server = std::move(server);
		m_world = m_server->world();
		Config config = loadConfig("server");
		
		for(int n = 0; n < m_world->entityCount(); n++) {
			Actor *actor = m_world->refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>();
		}

		if(!m_server->config().m_console_mode)
			m_controller.reset(new Controller(gfx::getWindowSize(), m_world, config.profiler_enabled));
	}

	bool ServerLoop::tick(double time_diff) {
		if(m_controller)
			m_controller->update(time_diff);
		m_server->beginFrame();

		m_world->simulate(time_diff);
		m_server->finishFrame();

		if(m_controller) {
			m_controller->updateView(time_diff);
			m_controller->draw();
		}

		return !isKeyDown(gfx::Key_esc) && !m_is_closing;
	}

}
