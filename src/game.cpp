/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <cstdio>

#include "io/io.h"

#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "audio/device.h"
#include "gfx/device.h"

using namespace gfx;
using namespace game;
using namespace io;

int safe_main(int argc, char **argv)
{
	audio::initSoundMap();
	Config config = loadConfig("game");
	game::loadData(true);

	audio::initDevice();

	audio::setListenerPos(float3(0, 0, 0));
	audio::setListenerVelocity(float3(0, 0, 0));
	audio::setUnits(16.66666666);

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);

	grabMouse(false);

	setBlendingMode(bmNormal);

	string map_name = "data/maps/mission05.mod";
	if(argc > 1)
		map_name = string("data/maps/") + argv[1];
	PWorld world(new World(map_name.c_str(), World::Mode::single_player));

	EntityRef actor_ref = world->addNewEntity<Actor>(float3(245, 128, 335), getProto("male", ProtoId::actor));

	IO io(config.resolution, world, actor_ref, config.profiler_enabled);

	double last_time = getTime();
	while(pollEvents() && !isKeyDown(Key_esc)) {
		double time = getTime();
		io.processInput();

		audio::tick();
		world->simulate((time - last_time) * config.time_multiplier);
		last_time = time;

		io.draw();

		swapBuffers();
		TextureCache::main_cache.nextFrame();
	}

/*	PTexture atlas = TextureCache::main_cache.atlas();
	Texture tex;
	atlas->download(tex);
	Saver("atlas.tga") & tex; */

	audio::freeDevice();
	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		audio::freeDevice();
		destroyWindow();

		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

