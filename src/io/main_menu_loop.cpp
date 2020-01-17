// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/world.h"
#include "ui/file_dialog.h"
#include "ui/message_box.h"
#include "ui/image_button.h"
#include <fwk/gfx/gl_device.h>
#include <fwk/gfx/gl_texture.h>
#include <fwk/gfx/opengl.h>

#include "io/main_menu_loop.h"
#include "io/game_loop.h"

#include "hud/multi_player_menu.h"

#include "net/client.h"
#include "net/server.h"
#include "audio/device.h"

#ifdef MessageBox // Yea.. TODO: remove windows.h from includes
#undef MessageBox
#endif
	
using namespace ui;
using namespace game;

namespace io {
	
	static const float s_transition_length = 0.7f;

	static PImageButton makeButton(const int2 &pos, const char *title) {
		char back_name[256];
		snprintf(back_name, sizeof(back_name), "btn/big/%d", rand() % 10 + 1);

		ImageButtonProto proto(back_name, "btn/big/Up", "btn/big/Dn", "transformers_30", FRect(0.25f, 0.05f, 0.95f, 0.95f));
		proto.sound_name = "butn_bigred";
		return make_shared<ImageButton>(pos, std::move(proto), title, ImageButton::mode_normal);
	}

	MainMenuLoop::MainMenuLoop()
		:Window(IRect(GlDevice::instance().windowSize()), ColorId::transparent), m_mode(mode_normal), m_next_mode(mode_normal) {
		m_back = res::getGuiTexture("back/flaminghelmet");
		m_loading = res::getGuiTexture("misc/worldm/OLD_moving");

		m_anim_pos = 0.0;
		m_blend_time = 1.0;

		IRect rect = localRect();
		m_back_rect = (IRect)FRect(
			float2(rect.center()) - float2(m_back->size()) * 0.5f,
			float2(rect.center()) + float2(m_back->size()) * 0.5f);

		m_single_player	= makeButton(m_back_rect.min() + int2(500, 70), "Single player");
		m_multi_player	= makeButton(m_back_rect.min() + int2(500, 115), "Multi player");
		m_create_server	= makeButton(m_back_rect.min() + int2(500, 160), "Create server");
		m_options		= makeButton(m_back_rect.min() + int2(500, 205), "Options");
		m_credits		= makeButton(m_back_rect.min() + int2(500, 250), "Credits");
		m_exit			= makeButton(m_back_rect.min() + int2(500, 295), "Exit");

		attach(m_single_player);
		attach(m_multi_player);
		attach(m_create_server);
		attach(m_options);
		attach(m_credits);
		attach(m_exit);

		startMusic();
	}
		
	MainMenuLoop::~MainMenuLoop() {
	}
		
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
			"data/music/gui/MX_ENV_MENU_MAIN1.mp3",
			"data/music/gui/MX_ENV_MENU_MAIN2.mp3",
			"data/music/gui/MX_MENU_WORLDMAP1.mp3",
			"data/music/gui/MX_MENU_WORLDMAP2.mp3"
		};

		m_music	= audio::playMusic(music_files[rand() % arraySize(music_files)], 1.0f);
		m_start_music_time = -1.0;
	}
		
	bool MainMenuLoop::onEvent(const Event &ev) {
		if(m_mode == mode_normal && ev.type == Event::button_clicked && ev.source->parent() == this) {
			if(ev.source == m_single_player.get()) {
				m_mode = mode_starting_single;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = make_shared<FileDialog>(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog, true);
			}
			else if(ev.source == m_create_server.get()) {
				m_mode = mode_starting_server;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = make_shared<FileDialog>(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog, true);
			}
			else if(ev.source == m_multi_player.get()) {
				FRect rect = FRect(float2(790, 550));
				auto window_size = GlDevice::instance().windowSize();
				rect += float2(window_size) * 0.5f - rect.size() * 0.5f;
				m_multi_menu = make_shared<hud::MultiPlayerMenu>(rect, window_size);
				m_sub_menu = m_multi_menu;
			//	m_mode = mode_starting_multi;
			}
			else if(ev.source == m_exit.get()) {
				stopMusic();
				m_mode = mode_transitioning;
				m_next_mode = mode_quitting;
				startTransition(Color(255, 255, 255, 0), Color(255, 255, 255, 255), trans_normal, 1.0f);
			}

			return true;
		}
		else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			string path = m_file_dialog->path();
			string map_name = FilePath(path).absolute().relative(FilePath("data/maps/").absolute());

			if(m_mode == mode_starting_single && ev.value) {
				m_future_world = std::async(std::launch::async,
					[map_name]() { return PWorld(new World(map_name, World::Mode::single_player)); } );
			}
			else if(m_mode == mode_starting_server && ev.value) {
				net::ServerConfig config;
				config.m_map_name = map_name;
				config.m_server_name = format("Test server #%", rand() % 256);
				m_server.reset(new net::Server(config));

				m_future_world = std::async(std::launch::async,
					[map_name]() { return PWorld(new World(map_name, World::Mode::server)); } );
			}
			
			m_mode = ev.value? mode_loading : mode_normal;
		}

		return false;
	}

	void MainMenuLoop::drawLoading(Renderer2D &out, float alpha) const {
		const char *text = "Loading";
		auto &font = res::getFont("transformers_30");
		FColor color(1.0f, 0.8f, 0.2f, alpha);

		int2 dims(m_loading->size());
		float2 center = float2(dims.x * 0.49f, dims.y * 0.49f);

		float scale = 1.0f + pow(sin(m_anim_pos * 0.5 * pi * 2.0), 8.0) * 0.1;

		FRect extents = font.draw(out, float2(0.0f, 0.0f), {color, ColorId::black, HAlign::right, VAlign::center}, text);

		out.pushViewMatrix();
		out.mulViewMatrix(translation(extents.ex() + 8.0f + center.x, 0.0f, 0.0f));
		out.mulViewMatrix(scaling(scale));
		out.mulViewMatrix(rotation(float3(0, 0, 1), m_anim_pos * 2.0f * pi));
		out.mulViewMatrix(translation(-center.x, -center.y, 0.0f));

		out.addFilledRect(IRect(dims), {m_loading, color});
		out.popViewMatrix();
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
				startTransition(Color(0, 0, 0, 255), Color(0, 0, 0, 0), trans_left, s_transition_length);
			}
			return true;
		}

		if(m_mode == mode_loading) {
			PLoop new_loop;

			// TODO: properly handle errors here;
			// Logic has to be simplified for rollbacking to work
			{
				if(m_future_world.valid() && m_future_world.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
					if(m_client) {
						PWorld world = m_future_world.get();
						m_client->setWorld(world);
						new_loop.reset(new GameLoop(std::move(m_client), true));
					}
					else if(m_server) {
						PWorld world = m_future_world.get();
						m_server->setWorld(world);
						new_loop.reset(new GameLoop(std::move(m_server), true));
					}
					else {
						new_loop.reset(new GameLoop(m_future_world.get(), true));
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

				startTransition(Color(0, 0, 0, 0), Color(0, 0, 0, 255), trans_right, s_transition_length);
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
			process(GlDevice::instance().inputState());
		if(m_sub_menu) {
			if(m_mode != mode_transitioning) {
				auto &device = GlDevice::instance();
				for(auto &event: device.inputEvents())
					m_sub_menu->handleInput(event);
			}

			m_sub_menu->update(time_diff);
			if(!m_sub_menu->isVisible() && !m_sub_menu->isShowing())
				m_sub_menu.reset();

			if(m_multi_menu->isClientReady()) {
				m_multi_menu->setVisible(false);
				m_client = std::move(m_multi_menu->getClient());
				const string map_name = m_client->levelInfo().map_name;

				m_future_world = std::async(std::launch::async,
						[map_name]() { return PWorld(new World(map_name, World::Mode::client)); } );
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

		m_anim_pos += time_diff;
		if(m_anim_pos > 1.0)
			m_anim_pos -= 1.0;

		return m_mode != mode_quitting;
	}

	void MainMenuLoop::onDraw() {
		if(m_sub_loop && m_mode != mode_transitioning) {
			m_sub_loop->draw();
			return;
		}

		clearColor(Color(0, 0, 0));
		IRect viewport(GlDevice::instance().windowSize());
		Renderer2D renderer(viewport, Orient2D::y_down);

		renderer.addFilledRect(m_back_rect, m_back);
		Window::draw(renderer);

		renderer.setViewPos(float2());
		if(m_sub_menu)
			m_sub_menu->draw(renderer);

		if(m_mode == mode_loading) {
			renderer.setViewPos(-(viewport.size() - int2(180, 50)));
			drawLoading(renderer, 1.0);
		}
		renderer.render();
	}

}
