// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "audio/device.h"
#include "game/base.h"
#include "io/game_loop.h"
#include "io/main_menu_loop.h"
#include "net/server.h"
#include "res_manager.h"
#include "sys/config.h"
#include "sys/gfx_device.h"
#include "sys/libs_msvc.h"

#include <fwk/libs_msvc.h>
#include <fwk/vulkan/vulkan_window.h>

#ifdef FWK_PLATFORM_WINDOWS
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "mpg123.lib")
#pragma comment(lib, "OpenAL32.lib")
#endif

using namespace game;

static bool s_is_closing = false;

void ctrlCHandler() {
	printf("Closing...\n");
	s_is_closing = true;
}

struct GameApp {
	GameApp(GfxDevice &gfx_device, io::PLoop main_loop)
		: m_gfx_device(gfx_device), m_main_loop(std::move(main_loop)) {}

	bool main_loop() {
		double time = getTime();
		double time_diff = (time - m_last_time);
		m_last_time = time;

		if(s_is_closing)
			m_main_loop->exit();
		if(!m_main_loop->tick(time_diff))
			return false;

		int2 window_size = m_gfx_device.window_ref->size();
		Canvas2D canvas(IRect(window_size), Orient2D::y_up);
		m_main_loop->draw(canvas);
		m_gfx_device.drawFrame(canvas).check();
		TextureCache::instance().nextFrame();
		audio::tick();
		return true;
	}

	static bool main_loop(VulkanWindow &, void *ptr) { return ((GameApp *)ptr)->main_loop(); }

  private:
	GfxDevice &m_gfx_device;
	io::PLoop m_main_loop;
	double m_last_time = getTime() - 1.0f / 60.0f;
};

Ex<int> exMain(int argc, char **argv) {
	Config config("game");

	// TODO: don't use rand()
	srand((int)getTime());

	net::ServerConfig server_config;
	string map_name;
	bool init_audio = true;

	for(int a = 1; a < argc; a++) {
		if(strcmp(argv[a], "-res") == 0) {
			ASSERT(a + 2 < argc);
			config.resolution = {atoi(argv[a + 1]), atoi(argv[a + 2])};
			ASSERT(config.resolution.x >= 0 && config.resolution.y >= 0);
			a += 2;
		} else if(strcmp(argv[a], "-pos") == 0) {
			ASSERT(a + 2 < argc);
			int2 window_pos(atoi(argv[a + 1]), atoi(argv[a + 2]));
			ASSERT(window_pos.x >= 0 && window_pos.y >= 0);
			config.window_pos = window_pos;
			a += 2;
		} else if(strcmp(argv[a], "-nosound") == 0)
			init_audio = false;
		else if(strcmp(argv[a], "-fullscreen") == 0)
			config.fullscreen_on = true;
		else if(strcmp(argv[a], "-server") == 0) {
			ASSERT(a + 1 < argc);
			auto xml_config = std::move(XmlDocument::load(argv[a + 1]).get()); // TODO
			auto server_node = xml_config.child("server");
			ASSERT(server_node);

			server_config = net::ServerConfig(server_node);
			ASSERT(server_config.isValid());
			init_audio = false;
			a++;
		} else {
			map_name = argv[a];
		}
	}
	config.resolution = vmax(config.resolution, int2(640, 480));

	if(init_audio)
		audio::initDevice();

	Dynamic<GfxDevice> gfx_device;
	Dynamic<TextureCache> tex_cache;
	audio::initSoundMap();

	bool console_mode = server_config.isValid() && server_config.m_console_mode;
	if(!console_mode) {
		gfx_device = EX_PASS(GfxDevice::create("game", config));
		tex_cache.emplace(*gfx_device->device_ref);
	}

	ResManager res_mgr(gfx_device ? gfx_device->device_ref : Maybe<VDeviceRef>(), console_mode);
	game::loadData(true);
	io::PLoop main_loop;

	// TODO: if errors happen here, run menu normally
	if(server_config.isValid()) {
		map_name = server_config.m_map_name;
		printf("Creating server: %s (map: %s)\n", server_config.m_server_name.c_str(),
			   map_name.c_str());
		net::PServer server(new net::Server(server_config));
		PWorld world(new World(map_name, World::Mode::server));
		server->setWorld(world);
		main_loop.reset(new io::GameLoop(gfx_device.get(), std::move(server), false));
		if(console_mode) {
			handleCtrlC(ctrlCHandler);
			printf("Press Ctrl+C to exit...\n");
		}
	} else if(!map_name.empty()) {
		DASSERT(gfx_device);
		printf("Loading map: %s\n", map_name.c_str());
		game::PWorld world(new World(map_name, World::Mode::single_player));
		main_loop.reset(new io::GameLoop(*gfx_device, world, false));
	}

	if(!main_loop) {
		if(console_mode)
			gfx_device = EX_PASS(GfxDevice::create("game", config));
		main_loop.reset(new io::MainMenuLoop(*gfx_device));
	}

	GameApp app(*gfx_device, std::move(main_loop));
	gfx_device->window_ref->runMainLoop(&GameApp::main_loop, &app);

	/*	PTexture atlas = tex_cache.atlas();
	Image tex;
	atlas->download(tex);
	Saver("atlas.tga") << tex;*/

	return 0;
}

int main(int argc, char **argv) {
	auto result = exMain(argc, argv);
	if(!result) {
		result.error().print();
		return 1;
	}
	return *result;
}
