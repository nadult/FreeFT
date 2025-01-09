// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "io/game_loop.h"
#include "game/actor.h"
#include "game/brain.h"
#include "game/game_mode.h"
#include "game/single_player_mode.h"
#include "game/world.h"
#include "io/controller.h"
#include "net/client.h"
#include "net/server.h"

using namespace game;

namespace io {

static const float s_transition_length = 0.7f;

GameLoop::GameLoop(const char *config_name, bool from_main)
	: m_config(config_name), m_from_main_menu(from_main) {
	if(from_main) {
		m_mode = mode_transitioning;
		m_next_mode = mode_normal;
		startTransition(Color(0, 0, 0, 255), Color(0, 0, 0, 0), trans_right, s_transition_length);
	} else {
		m_mode = mode_normal;
		m_next_mode = mode_normal;
	}
}

GameLoop::GameLoop(GfxDevice *gfx_device, net::PServer &&server, bool from_main)
	: GameLoop("server", from_main) {
	DASSERT(server && server->world());
	m_server = std::move(server);
	m_world = m_server->world();

	if(!m_server->config().m_console_mode) {
		DASSERT(gfx_device);
		m_controller.reset(new Controller(*gfx_device, m_world, m_config.profiler_on));
	}
}

GameLoop::GameLoop(GfxDevice &gfx_device, net::PClient &&client, bool from_main)
	: GameLoop("client", from_main) {
	DASSERT(client && client->world());
	m_client = std::move(client);
	m_world = m_client->world();

	//TODO: wait until initial entity information is loaded?
	m_controller.reset(new Controller(gfx_device, m_world, m_config.profiler_on));
}

GameLoop::GameLoop(GfxDevice &gfx_device, game::PWorld world, bool from_main)
	: GameLoop("game", from_main) {
	DASSERT(world && world->mode() == World::Mode::single_player);
	m_world = world;
	m_world->assignGameMode<SinglePlayerMode>(Character("Player", "CORE_prefab2", "male"));
	m_controller.reset(new Controller(gfx_device, m_world, m_config.profiler_on));
}

GameLoop::~GameLoop() {}

bool GameLoop::onTick(double time_diff) {
	double multiplier = !m_world->isClient() && !m_world->isServer() && m_controller ?
							m_controller->timeMultiplier() :
							1.0;

	//TODO: handle change of map
	if(m_controller)
		m_controller->update(time_diff);
	if(m_server)
		m_server->beginFrame();
	if(m_client)
		m_client->beginFrame();

	m_world->simulate(time_diff * multiplier);

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
			m_next_mode = to_main ? mode_exiting_to_main : mode_exiting_to_system;

			if(to_main)
				startTransition(Color(0, 0, 0, 0), ColorId::black, trans_left, s_transition_length);
			else
				startTransition(Color(255, 255, 255, 0), ColorId::white, trans_normal, 1.0f);
		}
	}

	if(m_mode == mode_exiting_to_system) {
		if(m_controller)
			m_controller.reset();
		if(m_world)
			m_world.reset();
		if(m_client)
			m_client.reset();
		if(m_server)
			m_server.reset();
		::exit(0);
	}

	return m_mode != mode_exiting_to_main;
}

void GameLoop::onDraw(Canvas2D &canvas) {
	if(m_controller)
		m_controller->draw(canvas);
}

void GameLoop::onTransitionFinished() {
	DASSERT(m_mode == mode_transitioning);
	m_mode = m_next_mode;
}

}
