// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "sys/config.h"
#include "audio/device.h"
#include "game/base.h"

#include "io/main_menu_loop.h"
#include "io/game_loop.h"
#include "io/controller.h"
#include "net/server.h"
#include <fwk/gfx/gl_device.h>
#include "res_manager.h"

using namespace game;

static bool s_is_closing = false;

void ctrlCHandler() {
	printf("Closing...\n");
	s_is_closing = true;
}

static io::PLoop s_main_loop;
static double s_last_time = getTime() - 1.0f / 60.0f;

static bool main_loop(GlDevice &device, void*) {
	double time = getTime();
	double time_diff = (time - s_last_time);
	s_last_time = time;

	if(s_is_closing)
		s_main_loop->exit();
	if(!s_main_loop->tick(time_diff))
		return false;
	s_main_loop->draw();

	TextureCache::instance().nextFrame();
	audio::tick();

	return true;
}

int main(int argc, char **argv) {
	Config config("game");

	srand((int)getTime());

	//adjustWindowSize(config.resolution, config.fullscreen_on);
	int2 res = config.resolution;

	int2 window_pos(-1, -1);
	net::ServerConfig server_config;
	string map_name;
	bool init_audio = true;
	bool fullscreen = config.fullscreen_on;

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
		else if(strcmp(argv[a], "-nosound") == 0)
			init_audio = false;
		else if(strcmp(argv[a], "-fullscreen") == 0)
			fullscreen = true;
		else if(strcmp(argv[a], "-server") == 0) {
			ASSERT(a + 1 < argc);
			auto xml_config = std::move(XmlDocument::load(argv[a + 1]).get()); // TODO
			auto server_node = xml_config.child("server");
			ASSERT(server_node);

			server_config = net::ServerConfig(server_node);
			ASSERT(server_config.isValid());

			init_audio = false;
			a++;
		}
		else {
			map_name = argv[a];
		}
	}
	res = vmax(res, int2(640, 480));

	if(platform == Platform::html)
		map_name = "demo_map.xml";

	if(init_audio)
		audio::initDevice();

	GlDevice gl_device;
	TextureCache tex_cache;
	audio::initSoundMap();

	bool console_mode = server_config.isValid() && server_config.m_console_mode;
	if(!console_mode)
		createWindow("game", gl_device, res, window_pos, fullscreen);
	
	ResManager res_mgr(console_mode);
	game::loadData(true);

	// TODO: if errors happen here, run menu normally
	if(server_config.isValid()) {
		map_name = server_config.m_map_name;
		printf("Creating server: %s (map: %s)\n", server_config.m_server_name.c_str(), map_name.c_str());
		net::PServer server(new net::Server(server_config));
		PWorld world(new World(map_name, World::Mode::server));
		server->setWorld(world);
		s_main_loop.reset(new io::GameLoop(std::move(server), false));
#ifndef FWK_PLATFORM_HTML
		if(server_config.m_console_mode)
			handleCtrlC(ctrlCHandler);
#endif
		if(console_mode)
			printf("Press Ctrl+C to exit...\n");
	}
	else if(!map_name.empty()) {
		printf("Loading map: %s\n", map_name.c_str());
		game::PWorld world(new World(map_name, World::Mode::single_player));
		s_main_loop.reset(new io::GameLoop(world, false));
	}

	if(!s_main_loop) {
		if(console_mode)
			createWindow("game", gl_device, res, window_pos, config.fullscreen_on);
		s_main_loop.reset(new io::MainMenuLoop);
	}

	gl_device.runMainLoop(main_loop);
	s_main_loop.reset(nullptr);

/*	PTexture atlas = tex_cache.atlas();
	Image tex;
	atlas->download(tex);
	Saver("atlas.tga") << tex;*/

	return 0;
}
