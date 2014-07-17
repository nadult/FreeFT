/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/game_loop.h"
#include "io/controller.h"
#include "gfx/device.h"
#include "game/actor.h"
#include "game/actor_ai.h"
#include "game/world.h"
#include "game/game_mode.h"
#include "game/single_player_mode.h"
#include "net/client.h"
#include "net/server.h"

using namespace game;

namespace io {

	static const float s_transition_length = 0.7f;
		
	GameLoop::GameLoop(const char *config_name, bool from_main)
		  :m_config(loadConfig(config_name)), m_from_main_menu(from_main) {
		if(from_main) {
			m_mode = mode_transitioning;
			m_next_mode = mode_normal;
			startTransition(Color(0, 0, 0, 255), Color(0, 0, 0, 0), trans_right, s_transition_length);
		}
		else {
			m_mode = mode_normal;
			m_next_mode = mode_normal;
		}
	}
		
	GameLoop::GameLoop(net::PServer &&server, bool from_main)
		  :GameLoop("server", from_main) {
		DASSERT(server && server->world());
		m_server = std::move(server);
		m_world = m_server->world();
		
		for(int n = 0; n < m_world->entityCount(); n++) {
			Actor *actor = m_world->refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>(m_world);
		}

		if(!m_server->config().m_console_mode)
			m_controller.reset(new Controller(gfx::getWindowSize(), m_world, m_config.profiler_enabled));
	}

	GameLoop::GameLoop(net::PClient &&client, bool from_main)
		  :GameLoop("client", from_main) {
		DASSERT(client && client->world());
		m_client = std::move(client);
		m_world = m_client->world();

		//TODO: wait until initial entity information is loaded?
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, m_config.profiler_enabled));
	}

	GameLoop::GameLoop(game::PWorld world, bool from_main)
		  :GameLoop("game", from_main) {
		DASSERT(world && world->mode() == World::Mode::single_player);
		m_world = world;
		m_world->assignGameMode<SinglePlayerMode>(Character("Player", "CORE_prefab2", "female"));
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, m_config.profiler_enabled));
	}

	GameLoop::~GameLoop() { }
		
	bool GameLoop::onTick(double time_diff) {
		//TODO: handle change of map
		if(m_controller)
			m_controller->update(time_diff);
		if(m_server)
			m_server->beginFrame();
		if(m_client)
			m_client->beginFrame();

		m_world->simulate(time_diff * m_config.time_multiplier);

		if(m_server)
			m_server->finishFrame();
		if(m_client)
			m_client->finishFrame();
		if(m_controller)
			m_controller->updateView(time_diff);

		if(m_mode == mode_normal && m_controller) {
			int exit_request = m_controller->exitRequested();
			if(exit_request) {
				m_mode = mode_transitioning;
				bool to_main = exit_request == 1 && m_from_main_menu;
				m_next_mode = to_main? mode_exiting_to_main : mode_exiting_to_system;

				if(to_main)
					startTransition(Color(0, 0, 0, 0), Color::black, trans_left, s_transition_length);
				else
					startTransition(Color(255, 255, 255, 0), Color::white, trans_normal, 1.0f);
			}
		}

		if(m_mode == mode_exiting_to_system) {
			if(m_controller)
				m_controller.reset(nullptr);
			if(m_world)
				m_world.reset();
			if(m_client)
				m_client.reset(nullptr);
			if(m_server)
				m_server.reset(nullptr);
			::exit(0);
		}

		return m_mode != mode_exiting_to_main;
	}
		
	void GameLoop::onDraw() {
		if(m_controller)
			m_controller->draw();
	}
		
	void GameLoop::onTransitionFinished() {
		DASSERT(m_mode == mode_transitioning);
		m_mode = m_next_mode;
	}

}

