/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/world.h"
#include "ui/file_dialog.h"
#include "ui/message_box.h"
#include "ui/image_button.h"
#include "io/main_menu_loop.h"
#include "io/single_player_loop.h"
#include "io/multi_player_loop.h"
#include "io/server_loop.h"
#include "gfx/device.h"
#include "gfx/opengl.h"
#include "audio/device.h"
#include "net/client.h"
#include "net/server.h"

#include "hud/hud.h"

#ifdef MessageBox // Yea.. TODO: remove windows.h from includes
#undef MessageBox
#endif
	
using namespace gfx;
using namespace ui;
using namespace game;

namespace io {

	static PImageButton makeButton(const int2 &pos, const char *title) {
		char back_name[256];
		snprintf(back_name, sizeof(back_name), "btn/big/%d", rand() % 10 + 1);

		ImageButtonProto proto(back_name, "btn/big/Up", "btn/big/Dn", "transformers_30", FRect(0.25f, 0.05f, 0.95f, 0.95f));
		proto.sound_name = "butn_bigred";
		return new ImageButton(pos, proto, title, ImageButton::mode_normal);
	}

	MainMenuLoop::MainMenuLoop() :Window(IRect({0, 0}, gfx::getWindowSize()), Color::transparent), m_mode(mode_normal) {
		m_back = gfx::DTexture::gui_mgr["back/flaminghelmet"];
		m_loading = gfx::DTexture::gui_mgr["misc/worldm/OLD_moving"];

		m_anim_pos = 0.0;
		m_blend_time = 1.0;
		m_timer = 0.0;

		IRect rect = localRect();
		m_back_rect = (IRect)FRect(
			float2(rect.center()) - float2(m_back->dimensions()) * 0.5f,
			float2(rect.center()) + float2(m_back->dimensions()) * 0.5f);

		m_single_player	= makeButton(m_back_rect.min + int2(500, 70), "Single player");
		m_multi_player	= makeButton(m_back_rect.min + int2(500, 115), "Multi player");
		m_create_server	= makeButton(m_back_rect.min + int2(500, 160), "Create server");
		m_options		= makeButton(m_back_rect.min + int2(500, 205), "Options");
		m_credits		= makeButton(m_back_rect.min + int2(500, 250), "Credits");
		m_exit			= makeButton(m_back_rect.min + int2(500, 295), "Exit");

		attach(m_single_player.get());
		attach(m_multi_player.get());
		attach(m_create_server.get());
		attach(m_options.get());
		attach(m_credits.get());
		attach(m_exit.get());

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

		m_music	= audio::playMusic(music_files[rand() % COUNTOF(music_files)], 1.0f);
		m_start_music_time = -1.0;
	}
		
	bool MainMenuLoop::onEvent(const Event &ev) {
		if(m_mode == mode_normal && ev.type == Event::button_clicked && ev.source->parent() == this) {
			if(ev.source == m_single_player.get()) {
				m_mode = mode_starting_single;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = new FileDialog(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog.get(), true);
			}
			else if(ev.source == m_create_server.get()) {
			//	m_mode = mode_starting_server;
			//	IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			//	m_file_dialog = new FileDialog(dialog_rect, "Select map", FileDialogMode::opening_file);
			//	m_file_dialog->setPath("data/maps/");
			//	attach(m_file_dialog.get(), true);
			}
			else if(ev.source == m_multi_player.get()) {
				FRect rect = FRect(float2(750, 550));
				rect += float2(gfx::getWindowSize()) * 0.5f - rect.size() * 0.5f;
				m_multi_menu = new hud::MultiPlayerMenu(rect, hud::defaultStyle());
				m_sub_menu = m_multi_menu.get();
			//	m_mode = mode_starting_multi;
			}
			else if(ev.source == m_exit.get()) {
				stopMusic();
				m_mode = mode_quitting;
				m_timer = 1.0;
			}

			return true;
		}
		else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			string path = m_file_dialog->path();

			if(m_mode == mode_starting_single && ev.value) {
				m_future_world = std::async(std::launch::async, [path]() { return createWorld(path); } );
			}
			
			m_mode = ev.value? mode_loading : mode_normal;
		}

		return false;
	}

	void MainMenuLoop::drawLoading(const int2 &pos, float alpha) const {
		const char *text = "Loading";
		gfx::PFont font = gfx::Font::mgr["transformers_30"];
		Color color(1.0f, 0.8f, 0.2f, alpha);

		lookAt(-pos);

		int2 dims(m_loading->dimensions());
		float2 center = float2(dims.x * 0.49f, dims.y * 0.49f);

		float scale = 1.0f + pow(sin(m_anim_pos * 0.5 * constant::pi * 2.0), 8.0) * 0.1;

		FRect extents = font->draw(float2(0.0f, 0.0f), {color, Color::black, HAlign::right, VAlign::center}, text);

		glPushMatrix();
		glTranslatef(extents.max.x + 8.0f + center.x, 0.0f, 0.0f);

		glScalef(scale, scale, scale);
		glRotated(m_anim_pos * 360.0, 0, 0, 1);
		glTranslatef(-center.x, -center.y, 0);

		m_loading->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		drawQuad(int2(0, 0), dims, color);
		glPopMatrix();
	}

	bool MainMenuLoop::tick(double time_diff) {
		using namespace gfx;

		if(m_sub_loop) {
			if(!m_sub_loop->tick(time_diff))
				m_sub_loop.reset(nullptr);
			return true;
		}

		if(m_mode == mode_loading) {
			PLoop new_loop;

			try {
				if(m_future_world.valid() && m_future_world.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
					if(m_client) {
						PWorld world = m_future_world.get();
						m_client->updateWorld(world);
						new_loop.reset(new MultiPlayerLoop(std::move(m_client)));
					}
					else if(m_server) {
					}
					else {
						new_loop.reset( new SinglePlayerLoop(m_future_world.get()) );
					}
				}
			}
			catch(const Exception &ex) {
				PFont font = gfx::Font::mgr[WindowStyle::fonts[1]];
				IRect extents = font->evalExtents(ex.what());
				int2 pos = rect().center(), size(min(rect().width(), extents.width() + 50), 100);

				PMessageBox message_box(new ui::MessageBox(IRect(pos - size / 2, pos + size / 2), ex.what(), MessageBoxMode::ok));
				attach(message_box.get());
				new_loop.reset(nullptr);
				m_mode = mode_normal;
			}
			
			if(new_loop) {
				m_sub_loop = std::move(new_loop);	
				m_mode = mode_normal;
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

		if(m_mode != mode_quitting && !m_sub_menu)
			process();
		if(m_sub_menu) {
			m_sub_menu->update(true, time_diff);
			if(!m_sub_menu->isVisible() && !m_sub_menu->isShowing())
				m_sub_menu.reset();

			if(m_multi_menu->isClientReady()) {
				m_multi_menu->setVisible(false);
				m_client = std::move(m_multi_menu->getClient());
				Replicator *replicator = dynamic_cast<Replicator*>(m_client.get());
				const string map_name = m_client->levelInfo().map_name;

				m_future_world = std::async(std::launch::async,
						[map_name, replicator]() { return PWorld(new World(map_name, World::Mode::client, replicator)); } );
				m_mode = mode_loading;
			}
		}

		if(m_client) {
			m_client->beginFrame();
			m_client->finishFrame();
		}

		clear(Color(0, 0, 0));
		m_back->bind();
		drawQuad(m_back_rect.min, m_back_rect.size());
	
		lookAt({0, 0});
		draw();

		lookAt({0, 0});
		if(m_sub_menu)
			m_sub_menu->draw();

		if(m_mode == mode_loading)
			drawLoading(gfx::getWindowSize() - int2(180, 50), 1.0);

		lookAt({0, 0});

		if(m_mode == mode_quitting) {
			DTexture::unbind();
			m_timer -= time_diff / m_blend_time;
			if(m_timer < 0.0) {
				m_timer = 0.0;
				m_mode = mode_quit;
			}
			drawQuad({0, 0}, gfx::getWindowSize(), Color(1.0f, 1.0f, 1.0f, 1.0f - m_timer));
		}

		m_anim_pos += time_diff;
		if(m_anim_pos > 1.0)
			m_anim_pos -= 1.0;

		return m_mode != mode_quit;
	}

}
