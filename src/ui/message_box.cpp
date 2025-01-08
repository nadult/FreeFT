// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "ui/message_box.h"
#include "ui/button.h"
#include "ui/text_box.h"

namespace ui {

MessageBox::MessageBox(const IRect &rect, const char *message, MessageBoxMode mode)
	: Window(rect), m_mode(mode) {
	int w = width(), h = height();

	PTextBox text_box = make_shared<TextBox>(IRect(5, 5, w - 5, h - 30), message);
	text_box->setFont(WindowStyle::fonts[1]);
	attach(text_box);

	if(mode == MessageBoxMode::ok) {
		attach(make_shared<Button>(IRect(w / 2 - 40, h - 25, w / 2 + 40, h - 5), "ok", 0));
	} else {
		attach(make_shared<Button>(IRect(w / 2 - 50, h - 25, w / 2 - 2, h - 5),
								   mode == MessageBoxMode::yes_no ? "yes" : "ok", 1));
		attach(make_shared<Button>(IRect(w / 2 + 2, h - 25, w / 2 + 50, h - 5),
								   mode == MessageBoxMode::yes_no ? "no" : "cancel", 0));
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

void MessageBox::drawContents(Canvas2D &out) const {
	drawWindow(out, IRect({0, 0}, rect().size()), WindowStyle::gui_dark, 3);
}

}
