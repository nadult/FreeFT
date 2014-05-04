/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/profiler.h"
#include "sys/platform.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "audio/device.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"
#include "game/base.h"

#include "io/main_menu_loop.h"

using namespace gfx;
using namespace game;
using namespace io;


int safe_main(int argc, char **argv)
{
	Config config = loadConfig("game");

	srand((int)getTime());
	audio::initSoundMap();
	game::loadData(true);
	audio::initDevice();

	int2 resolution = max(config.resolution, int2(800, 500));
	createWindow(resolution, config.fullscreen);
	setWindowTitle("FreeFT alpha (built " __DATE__ " " __TIME__ ")");

	grabMouse(false);
	setBlendingMode(bmNormal);

	io::PLoop main_loop(new io::MainMenuLoop);

	double last_time = getTime() - 1.0 / 60.0;
	while(pollEvents()) {
		double time = getTime();
		double time_diff = (time - last_time);
		last_time = time;

		if(!main_loop->tick(time_diff))
			break;

		TextureCache::main_cache.nextFrame();
		swapBuffers();
		audio::tick();
	}

	main_loop.reset(nullptr);

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

