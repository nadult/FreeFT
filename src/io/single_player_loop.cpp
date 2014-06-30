/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/single_player_loop.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/trigger.h"
#include "game/character.h"
#include "sys/config.h"
#include "game/single_player_mode.h"

using namespace game;

namespace io {
	
	game::PWorld createWorld(const string &map_name) {
		game::PWorld world(new World(map_name, World::Mode::single_player));
		return world;
	}

	SinglePlayerLoop::SinglePlayerLoop(game::PWorld world) {
		DASSERT(world && world->mode() == World::Mode::single_player);
		m_world = world;
			
		m_world->assignGameMode<SinglePlayerMode>(Character("Player", "CORE_prefab2", "male"));

		Config config = loadConfig("game");
		m_time_multiplier = config.time_multiplier;
		
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, config.profiler_enabled));
	}

	bool SinglePlayerLoop::tick(double time_diff) {
		m_controller->update(time_diff);

		time_diff *= m_time_multiplier;
		m_world->simulate(time_diff);
		m_controller->updateView(time_diff);

		m_controller->draw();
		return !gfx::isKeyPressed(gfx::Key_esc);
	}

}
