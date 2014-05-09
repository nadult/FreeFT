/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MAIN_MENU_LOOP_H
#define IO_MAIN_MENU_LOOP_H

#include "io/loop.h"
#include "ui/window.h"
#include <future>
#include <thread>

namespace io {

	class MainMenuLoop: public ui::Window, public Loop {
	public:
		MainMenuLoop();
		~MainMenuLoop();

		bool tick(double time_diff) override;
		bool onEvent(const ui::Event &ev) override;

		void stopMusic();
		void startMusic();
		void drawLoading(const int2 &pos, float alpha = 1.0f) const;

	private:
		ui::PImageButton m_single_player;
		ui::PImageButton m_multi_player;
		ui::PImageButton m_create_server;
		ui::PImageButton m_options;
		ui::PImageButton m_credits;
		ui::PImageButton m_exit;
		
		ui::PFileDialog m_file_dialog;

		enum Mode {
			mode_normal,
			mode_starting_single,
			mode_starting_multi,
			mode_starting_server,
			mode_loading,
			mode_quitting,
			mode_quit,
		} m_mode;

		audio::PPlayback m_music;

		gfx::PTexture m_back, m_loading;
		IRect m_back_rect;
	
		std::future<game::PWorld> m_future_world;
		std::future<net::PClient> m_future_client;
		std::future<net::PServer> m_future_server;	
		PLoop m_sub_loop;

		double m_anim_pos;
		double m_timer, m_blend_time;
		double m_start_music_time;
	};

}

#endif
