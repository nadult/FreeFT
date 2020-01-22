// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "io/loop.h"
#include "ui/window.h"
#include <future>
#include "hud/layer.h"
#include "ui/loading_bar.h"

namespace io {

	class MainMenuLoop: public ui::Window, public Loop {
	public:
		MainMenuLoop();
		~MainMenuLoop();

		void stopMusic();
		void startMusic();

	private:
		bool onTick(double) override;
		bool onEvent(const ui::Event &ev) override;
		void onDraw() override;
		void onTransitionFinished() override;

		ui::PImageButton m_single_player;
		ui::PImageButton m_multi_player;
		ui::PImageButton m_create_server;
		ui::PImageButton m_options;
		ui::PImageButton m_credits;
		ui::PImageButton m_exit;
		
		ui::PFileDialog m_file_dialog;

		hud::PMultiPlayerMenu m_multi_menu;
	//	hud::PSinglePlayerMenu m_single_menu;
	//	hud::PServerMenu m_server_menu;
		hud::PHudLayer m_sub_menu;

		enum Mode {
			mode_normal,
			mode_transitioning,
			mode_starting_single,
			mode_starting_multi,
			mode_starting_server,
			mode_loading,
			mode_quitting,
		} m_mode, m_next_mode;


		audio::PPlayback m_music;

		PTexture m_back;
		IRect m_back_rect;
	
		std::future<game::PWorld> m_future_world;
		net::PClient m_client;
		net::PServer m_server;	
		PLoop m_sub_loop;
		ui::LoadingBar m_loading;

		double m_blend_time;
		double m_start_music_time;
	};

}
