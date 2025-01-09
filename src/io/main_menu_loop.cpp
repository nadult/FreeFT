// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/world.h"

#include "audio/device.h"
#include "hud/multi_player_menu.h"
#include "io/game_loop.h"
#include "io/main_menu_loop.h"
#include "net/client.h"
#include "net/server.h"
#include "sys/gfx_device.h"
#include "ui/file_dialog.h"
#include "ui/image_button.h"

#ifdef MessageBox // Yea.. TODO: remove windows.h from includes
#undef MessageBox
#endif

#include <fwk/io/file_system.h>
#include <fwk/vulkan/vulkan_window.h>

using namespace ui;
using namespace game;

namespace io {

static const float s_transition_length = 0.7f;

static PImageButton makeButton(const int2 &pos, const char *title) {
	char back_name[256];
	snprintf(back_name, sizeof(back_name), "btn/big/%d", rand() % 10 + 1);

	ImageButtonProto proto(back_name, "btn/big/Up", "btn/big/Dn", "transformers_30",
						   FRect(0.25f, 0.05f, 0.95f, 0.95f));
	proto.sound_name = "butn_bigred";
	return make_shared<ImageButton>(pos, std::move(proto), title, ImageButton::mode_normal);
}

MainMenuLoop::MainMenuLoop(GfxDevice &gfx_device)
	: Window(IRect(gfx_device.window_ref->size()), ColorId::transparent), m_gfx_device(gfx_device),
	  m_mode(mode_normal), m_next_mode(mode_normal) {
	m_back = res::getGuiTexture("back/flaminghelmet");

	m_blend_time = 1.0;

	IRect rect = localRect();
	m_back_rect = (IRect)FRect(float2(rect.center()) - float2(m_back->size2D()) * 0.5f,
							   float2(rect.center()) + float2(m_back->size2D()) * 0.5f);

	m_single_player = makeButton(m_back_rect.min() + int2(500, 70), "Single player");
	m_multi_player = makeButton(m_back_rect.min() + int2(500, 115), "Multi player");
	m_create_server = makeButton(m_back_rect.min() + int2(500, 160), "Create server");
	m_options = makeButton(m_back_rect.min() + int2(500, 205), "Options");
	m_credits = makeButton(m_back_rect.min() + int2(500, 250), "Credits");
	m_exit = makeButton(m_back_rect.min() + int2(500, 295), "Exit");

	attach(m_single_player);
	attach(m_multi_player);
	attach(m_create_server);
	attach(m_options);
	attach(m_credits);
	attach(m_exit);

	startMusic();
}

MainMenuLoop::~MainMenuLoop() {}

void MainMenuLoop::stopMusic() {
	if(m_music) {
		m_music->stop(m_blend_time);
		m_music.reset();
	}
}

void MainMenuLoop::startMusic() {
	if(m_music && m_music->isPlaying())
		return;

	const char *music_files[] = {
		"data/music/gui/MX_ENV_MENU_MAIN1.mp3", "data/music/gui/MX_ENV_MENU_MAIN2.mp3",
		"data/music/gui/MX_MENU_WORLDMAP1.mp3", "data/music/gui/MX_MENU_WORLDMAP2.mp3"};

	m_music = audio::playMusic(music_files[rand() % arraySize(music_files)], 1.0f);
	m_start_music_time = -1.0;
}

bool MainMenuLoop::onEvent(const Event &ev) {
	if(m_mode == mode_normal && ev.type == Event::button_clicked && ev.source->parent() == this) {
		if(ev.source == m_single_player.get()) {
			m_mode = mode_starting_single;
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog =
				make_shared<FileDialog>(dialog_rect, "Select map", FileDialogMode::opening_file);
			m_file_dialog->setPath("data/maps/");
			attach(m_file_dialog, true);
		} else if(ev.source == m_create_server.get()) {
			m_mode = mode_starting_server;
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog =
				make_shared<FileDialog>(dialog_rect, "Select map", FileDialogMode::opening_file);
			m_file_dialog->setPath("data/maps/");
			attach(m_file_dialog, true);
		} else if(ev.source == m_multi_player.get()) {
			FRect rect = FRect(float2(790, 550));
			auto window_size = m_gfx_device.window_ref->size();
			rect += float2(window_size) * 0.5f - rect.size() * 0.5f;
			m_multi_menu = make_shared<hud::MultiPlayerMenu>(rect, window_size);
			m_sub_menu = m_multi_menu;
			//	m_mode = mode_starting_multi;
		} else if(ev.source == m_exit.get()) {
			stopMusic();
			m_mode = mode_transitioning;
			m_next_mode = mode_quitting;
			startTransition(Color(255, 255, 255, 0), Color(255, 255, 255, 255), trans_normal, 1.0f);
		}

		return true;
	} else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
		string path = m_file_dialog->path();
		auto current = FilePath::current().get(); // TODO
		auto abs_path = FilePath(path).absolute(current);
		string map_name = abs_path.relative(FilePath("data/maps/").absolute(current));

		if(m_mode == mode_starting_single && ev.value) {
			m_future_world = std::async(std::launch::async, [map_name]() {
				return PWorld(new World(map_name, World::Mode::single_player));
			});
		} else if(m_mode == mode_starting_server && ev.value) {
			net::ServerConfig config;
			config.m_map_name = map_name;
			config.m_server_name = format("Test server #%", rand() % 256);
			m_server.reset(new net::Server(config));

			m_future_world = std::async(std::launch::async, [map_name]() {
				return PWorld(new World(map_name, World::Mode::server));
			});
		}

		m_mode = ev.value ? mode_loading : mode_normal;
	}

	return false;
}

void MainMenuLoop::onTransitionFinished() {
	DASSERT(m_mode == mode_transitioning);
	m_mode = m_next_mode;
}

bool MainMenuLoop::onTick(double time_diff) {
	if(m_mode == mode_transitioning && !isTransitioning())
		m_mode = mode_normal;

	if(m_sub_loop && m_mode == mode_normal) {
		if(!m_sub_loop->tick(time_diff)) {
			m_sub_loop.reset(nullptr);
			m_mode = mode_transitioning;
			m_next_mode = mode_normal;
			startTransition(Color(0, 0, 0, 255), Color(0, 0, 0, 0), trans_left,
							s_transition_length);
		}
		return true;
	}

	if(m_mode == mode_loading) {
		PLoop new_loop;

		// TODO: properly handle errors here;
		// Logic has to be simplified for rollbacking to work
		{
			if(m_future_world.valid() &&
			   m_future_world.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				if(m_client) {
					PWorld world = m_future_world.get();
					m_client->setWorld(world);
					new_loop.reset(new GameLoop(m_gfx_device, std::move(m_client), true));
				} else if(m_server) {
					PWorld world = m_future_world.get();
					m_server->setWorld(world);
					new_loop.reset(new GameLoop(&m_gfx_device, std::move(m_server), true));
				} else {
					new_loop.reset(new GameLoop(m_gfx_device, m_future_world.get(), true));
				}
			}
		}
		/*catch(const Exception &ex) {
				auto &font = res::getFont(WindowStyle::fonts[1]);
				IRect extents = font.evalExtents(ex.what());
				int2 pos = rect().center(), size(min(rect().width(), extents.width() + 50), 100);

				PMessageBox message_box(make_shared<ui::MessageBox>(IRect(pos - size / 2, pos + size / 2), ex.what(), MessageBoxMode::ok));
				attach(message_box);
				new_loop.reset(nullptr);
				m_mode = mode_normal;
			}*/

		if(new_loop) {
			m_sub_loop = std::move(new_loop);
			m_sub_loop->tick(time_diff);

			startTransition(Color(0, 0, 0, 0), Color(0, 0, 0, 255), trans_right,
							s_transition_length);
			m_mode = mode_transitioning;
			m_next_mode = mode_normal;
			stopMusic();
		}
	}

	if((!m_music || !m_music->isPlaying()) && m_start_music_time < 0.0)
		m_start_music_time = 5.0;

	if(m_start_music_time > 0.0) {
		m_start_music_time -= time_diff;
		if(m_start_music_time <= 0.0 && m_mode != mode_quitting && m_mode != mode_loading)
			startMusic();
	}

	if(m_mode != mode_quitting && m_mode != mode_transitioning && !m_sub_menu)
		process(m_gfx_device.window_ref->inputState());
	if(m_sub_menu) {
		if(m_mode != mode_transitioning) {
			for(auto &event : m_gfx_device.window_ref->inputEvents())
				m_sub_menu->handleInput(event);
		}

		m_sub_menu->update(time_diff);
		if(!m_sub_menu->isVisible() && !m_sub_menu->isShowing())
			m_sub_menu.reset();

		if(m_multi_menu->isClientReady()) {
			m_multi_menu->setVisible(false);
			m_client = std::move(m_multi_menu->getClient());
			const string map_name = m_client->levelInfo().map_name;

			m_future_world = std::async(std::launch::async, [map_name]() {
				return PWorld(new World(map_name, World::Mode::client));
			});
			m_mode = mode_loading;
		}
	}

	if(m_client) {
		m_client->beginFrame();
		m_client->finishFrame();
	}

	if(m_server) {
		m_server->beginFrame();
		m_server->finishFrame();
	}

	m_loading.animate(time_diff);

	return m_mode != mode_quitting;
}

void MainMenuLoop::onDraw(Canvas2D &canvas) {
	if(m_sub_loop && m_mode != mode_transitioning) {
		m_sub_loop->draw(canvas);
		return;
	}

	canvas.setMaterial(m_back);
	canvas.addFilledRect(m_back_rect);
	canvas.setMaterial({});
	Window::draw(canvas);

	canvas.setViewPos(float2());
	if(m_sub_menu)
		m_sub_menu->draw(canvas);

	if(m_mode == mode_loading)
		m_loading.draw(canvas, canvas.viewport().size() - int2(180, 50));
}

}
