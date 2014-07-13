/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_CONSOLE_H
#define IO_CONSOLE_H

#include "base.h"

namespace gfx { class Font; }

namespace io {

	//TODO: move Console to hud, use HudEditBox
	

	class ConsoleEditBox;

	class Console: public RefCounter {
	public:
		Console(const int2 &resolution);
		Console(const Console&) = delete;
		void operator=(const Console&) = delete;
		~Console();

		void open();
		void close();
		bool isOpened() const { return m_is_opened; }

		bool onInput(const InputEvent&);
		void update(double time_diff);
		void draw() const;

		const string getCommand();
		const int2 size() const;

		bool isMouseOver(const InputEvent&) const;

	protected:
		Ptr<gfx::Font> m_font;
		Ptr<ConsoleEditBox> m_edit_box;
		vector<string> m_commands;
		bool m_is_opened;
	};

}

#endif
