/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MAIN_MENU_LOOP_H
#define IO_MAIN_MENU_LOOP_H

#include "io/loop.h"
#include "ui/window.h"
#include <future>
#include <thread>
#include "hud/layer.h"

namespace io {

	class MainMenuLoop: public ui::Window, public Loop {
	public:
		MainMenuLoop();
		~MainMenuLoop();

		void stopMusic();
		void startMusic();
		void drawLoading(const int2 &pos, float alpha = 1.0f) const;

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

		PTexture m_back, m_loading;
		IRect m_back_rect;
	
		std::future<game::PWorld> m_future_world;
		net::PClient m_client;
		net::PServer m_server;	
		PLoop m_sub_loop;

		double m_anim_pos;
		double m_blend_time;
		double m_start_music_time;
	};

}

#endif
