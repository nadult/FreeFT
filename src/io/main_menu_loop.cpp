/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/file_dialog.h"
#include "ui/image_button.h"
#include "io/main_menu_loop.h"
#include "io/single_player_loop.h"
#include "io/multi_player_loop.h"
#include "io/server_loop.h"
#include "gfx/device.h"
#include "audio/device.h"

namespace io {

	using namespace gfx;
	using namespace ui;

	static PImageButton makeButton(const int2 &pos, const char *title) {
		char back_name[256];
		snprintf(back_name, sizeof(back_name), "btn/big/%d", rand() % 10 + 1);

		ImageButtonProto proto(back_name, "btn/big/Up", "btn/big/Dn", "transformers_30", FRect(0.25f, 0.05f, 0.95f, 0.95f));
		proto.sound_name = "butn_bigred";
		return new ImageButton(pos, proto, title, ImageButton::mode_normal);
	}

	const char *music_files[] = {
		"data/music/gui/MX_ENV_MENU_MAIN1.mp3",
		"data/music/gui/MX_ENV_MENU_MAIN2.mp3",
		"data/music/gui/MX_MENU_WORLDMAP1.mp3",
		"data/music/gui/MX_MENU_WORLDMAP2.mp3"
	};

	MainMenuLoop::MainMenuLoop() :Window(IRect({0, 0}, gfx::getWindowSize()), Color::transparent), m_mode(mode_normal), m_timer(0.0) {
		m_back = gfx::DTexture::gui_mgr["back/flaminghelmet"];

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

		m_music	= audio::playMusic(music_files[rand() % COUNTOF(music_files)], 1.0f);

		attach(m_single_player.get());
		attach(m_multi_player.get());
		attach(m_create_server.get());
		attach(m_options.get());
		attach(m_credits.get());
		attach(m_exit.get());
	}
		
	void MainMenuLoop::drawContents() const {
		using namespace gfx;

		m_back->bind();
		drawQuad(m_back_rect.min, m_back_rect.size());

		gfx::PFont title_font = gfx::Font::mgr["transformers_48"];
		title_font->drawShadowed(m_back_rect.min + int2(140, 90), Color(255, 200, 50), Color::black, "FreeFT");
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
				m_mode = mode_starting_server;
				IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
				m_file_dialog = new FileDialog(dialog_rect, "Select map", FileDialogMode::opening_file);
				m_file_dialog->setPath("data/maps/");
				attach(m_file_dialog.get(), true);
			}
			else if(ev.source == m_multi_player.get()) {
				m_sub_loop.reset(new MultiPlayerLoop("", 20001));
			}
			else if(ev.source == m_exit.get()) {
				m_mode = mode_quitting;
				m_timer = 1.0;
			}

			return true;
		}
		else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			if(m_mode == mode_starting_single && ev.value) {
				m_sub_loop.reset(new SinglePlayerLoop(m_file_dialog->path()));
			}
			else if(m_mode == mode_starting_server && ev.value) {
				m_sub_loop.reset(new ServerLoop(m_file_dialog->path()));
			}
			else if(m_mode == mode_starting_multi && ev.value) {
				m_sub_loop.reset(new MultiPlayerLoop("", 20001));
			}
			
			m_mode = mode_normal;	
		}

		return false;
	}

	bool MainMenuLoop::tick(double time_diff) {
		if(m_sub_loop) {
			if(!m_sub_loop->tick(time_diff))
				m_sub_loop.reset(nullptr);
			return true;
		}

		if(m_mode != mode_quitting)
			process();

		clear(Color(0, 0, 0));

		lookAt({0, 0});
		draw();

		lookAt({0, 0});
		if(m_mode == mode_quitting) {
			DTexture::bind0();
			m_timer -= time_diff * 2.0;
			if(m_timer < 0.0) {
				m_timer = 0.0;
				m_mode = mode_quit;
			}
			drawQuad({0, 0}, gfx::getWindowSize(), Color(1.0f, 1.0f, 1.0f, 1.0f - m_timer));
		}

		return m_mode != mode_quit;
	}

}
