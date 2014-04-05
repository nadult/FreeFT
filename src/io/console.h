/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef FREEFT_IO_CONSOLE_H
#define FREEFT_IO_CONSOLE_H

#include "base.h"

namespace gfx { class Font; }

namespace io {

	class ConsoleEditBox;

	class Console {
	public:
		Console(const int2 &resolution);
		~Console();

		void open();
		void close();
		bool isOpened() const { return m_is_opened; }

		void processInput();
		void draw() const;

		const string getCommand();
		const int2 size() const;

	protected:
		Ptr<gfx::Font> m_font;
		Ptr<ConsoleEditBox> m_edit_box;
		vector<string> m_commands;
		bool m_is_opened;
	};

}

#endif