/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "ui/message_box.h"
#include "ui/button.h"
#include "ui/text_box.h"


namespace ui {

	MessageBox::MessageBox(const IRect &rect, const char *message, MessageBoxMode::Type mode)
		:Window(rect), m_mode(mode) {
		int w = width(), h = height();

		PTextBox text_box = new TextBox(IRect(5, 5, w - 5, h - 30), message);
		text_box->setFont(s_font_names[1]);
		attach(text_box.get());

		if(mode == MessageBoxMode::ok) {
			attach(new Button(IRect(w/2 - 40, h - 25, w/2 + 40, h - 5), "ok", 0));
		}
		else {
			attach(new Button(IRect(w/2 - 50, h - 25, w/2 - 2, h - 5),
					mode == MessageBoxMode::yes_no? "yes" : "ok", 1));
			attach(new Button(IRect(w/2 + 2, h - 25, w/2 + 50, h - 5),
					mode == MessageBoxMode::yes_no? "no" : "cancel", 0));
		}
	}

	bool MessageBox::onEvent(const Event &event) {
		if(event.type == Event::button_clicked)
			close(event.value);
		else if(event.type == Event::escape)
			close(0);
		else
			return false;

		return true;
	}

	void MessageBox::drawContents() const {
		drawWindow(IRect({0, 0}, rect().size()), Color::gui_dark, 3);
	}

}
