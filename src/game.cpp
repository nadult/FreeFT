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

/*	double time = getTime();
	vector<int3> path;
	for(int n = 0; n < 1000; n++)
		path = world->naviMap().findPath(int3(160, 128, 328), int3(230, 128, 350));
	printf("Short: %.f usec\n", (getTime() - time) * 1000.0);

	time = getTime();
	for(int n = 0; n < 1000; n++)
		path = world->naviMap().findPath(int3(160, 128, 328), int3(430, 128, 476));
	printf(" Long: %.f usec\n", (getTime() - time) * 1000.0);*/

	for(int n = 0; n < world->entityCount(); n++) {
		Actor *actor = world->refEntity<Actor>(n);
		if(actor && actor->factionId() != 0)
			actor->attachAI<SimpleAI>();
	}

	EntityRef actor_ref = world->addNewEntity<Actor>(float3(295, 142, 292), getProto("male", ProtoId::actor));
	if( Actor *actor = world->refEntity<Actor>(actor_ref) ) {
		auto &inventory = actor->inventory();
		inventory.add(findProto("plasma_rifle", ProtoId::item_weapon), 1);
		inventory.add(findProto("laser_rifle", ProtoId::item_weapon), 1);
		inventory.add(findProto("power_armour", ProtoId::item_armour), 1);
	}
	

	game::WorldViewer viewer(world, actor_ref);
	IO io(config.resolution, world, viewer, actor_ref, config.profiler_enabled);

	double last_time = getTime();
	while(pollEvents() && !isKeyDown(Key_esc)) {
		double time = getTime();
		io.update();

		audio::tick();
		double time_diff = (time - last_time) * config.time_multiplier;
		world->simulate(time_diff);
		viewer.update(time_diff);
		last_time = time;

		io.draw();

		TextureCache::main_cache.nextFrame();
		swapBuffers();
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

