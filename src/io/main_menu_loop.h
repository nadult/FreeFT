/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_MAIN_MENU_LOOP_H
#define IO_MAIN_MENU_LOOP_H

#include "io/loop.h"
#include "ui/window.h"
#include "ui/image_button.h"
#include "ui/file_dialog.h"

namespace io {

	class MainMenuLoop: public ui::Window, public Loop {
	public:
		MainMenuLoop();

		bool tick(double time_diff) override;

		void drawContents() const override;
		bool onEvent(const ui::Event &ev) override;

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
			mode_quitting,
			mode_quit,
		} m_mode;

		audio::PPlayback m_music;

		gfx::PTexture m_back;
		IRect m_back_rect;
		PLoop m_sub_loop;
		double m_timer;
	};

}

#endif
