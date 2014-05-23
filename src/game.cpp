/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/profiler.h"
#include "sys/platform.h"
#include "sys/config.h"
#include "audio/device.h"
#include "gfx/device.h"
#include "game/base.h"

#include "io/main_menu_loop.h"
#include "io/single_player_loop.h"
#include "io/multi_player_loop.h"
#include "io/server_loop.h"
#include "net/server.h"

using namespace gfx;
using namespace game;
using namespace io;

static bool s_is_closing = false;

void ctrlCHandler() {
	printf("Closing...\n");
	s_is_closing = true;
}

void createWindow(const int2 &res, const int2 &pos, bool fullscreen) {
	createWindow(res, fullscreen);
	grabMouse(false);

	if(pos != int2(-1, -1))
		setWindowPos(pos);
	//TODO: date is refreshed only when game.o is being rebuilt
	setWindowTitle("FreeFT alpha (built " __DATE__ " " __TIME__ ")");
}

int safe_main(int argc, char **argv)
{
	Config config = loadConfig("game");

	srand((int)getTime());
	audio::initSoundMap();
	game::loadData(true);

	gfx::initDevice();
	audio::initDevice();

	int2 res = config.resolution;

	int2 window_pos(-1, -1);
	net::ServerConfig server_config;
	string map_name;

	for(int a = 1; a < argc; a++) {
		if(strcmp(argv[a], "-res") == 0) {
			ASSERT(a + 2 < argc);
			res.x = atoi(argv[a + 1]);
			res.y = atoi(argv[a + 2]);
			ASSERT(res.x >= 0 && res.y >= 0);
			a += 2;
		}
		else if(strcmp(argv[a], "-pos") == 0) {
			ASSERT(a + 2 < argc);
			window_pos.x = atoi(argv[a + 1]);
			window_pos.y = atoi(argv[a + 2]);
			ASSERT(window_pos.x >= 0 && window_pos.y >= 0);
			a += 2;
		}
		else if(strcmp(argv[a], "-server") == 0) {
			ASSERT(a + 1 < argc);
			XMLDocument xml_config;
			xml_config.load(argv[a + 1]);
			XMLNode server_node = xml_config.child("server");
			ASSERT(server_node);

			server_config = net::ServerConfig(server_node);
			ASSERT(server_config.isValid());
			a++;
		}
		else {
			map_name = string("data/maps/") + argv[a];
		}

	}
	res = max(res, int2(640, 480));

	bool console_mode = server_config.isValid() && server_config.m_console_mode;
	if(!console_mode)
		createWindow(res, window_pos, config.fullscreen);

	io::PLoop main_loop;

	try {
		if(server_config.isValid()) {
			printf("Creating server: %s (map: %s)\n", server_config.m_server_name.c_str(), server_config.m_map_name.c_str());
			net::PServer server(new net::Server(server_config));
			main_loop.reset(new io::ServerLoop(std::move(server)));
			if(server_config.m_console_mode)
				sys::handleCtrlC(ctrlCHandler);
			if(console_mode)
				printf("Press Ctrl+C to exit...\n");
		}
		else if(!map_name.empty()) {
			printf("Loading: %s\n", map_name.c_str());
			PWorld world = createWorld(map_name);
			main_loop.reset(new io::SinglePlayerLoop(world));
		}
	}
	catch(const Exception &ex) {
		printf("Failed: %s\n", ex.what());
	}

	if(!main_loop) {
		if(console_mode)
			createWindow(res, window_pos, config.fullscreen);
		main_loop.reset(new io::MainMenuLoop);
	}


	double last_time = getTime() - targetFrameTime();
	while(pollEvents()) {
		double time = getTime();
		double time_diff = (time - last_time);
		last_time = time;

		if(s_is_closing)
			main_loop->close();

		if(!main_loop->tick(time_diff))
			break;

		gfx::tick();
		audio::tick();
	}

	main_loop.reset(nullptr);

/*	PTexture atlas = TextureCache::main_cache.atlas();
	Texture tex;
	atlas->download(tex);
	Saver("atlas.tga") << tex;*/

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

