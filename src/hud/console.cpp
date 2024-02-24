// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/console.h"
#include "hud/edit_box.h"

namespace hud {

static constexpr int console_height = 22, max_command_length = 128;

HudConsole::HudConsole(const int2 &resolution)
	: HudLayer(FRect(0, 0, resolution.x, console_height), HudLayer::slide_top) {
	m_edit_box = make_shared<HudEditBox>(rect(), max_command_length, HudEditBox::mode_console);
	attach(m_edit_box);
	m_edit_box->setStyle(getStyle(HudStyleId::console));
	setVisible(false, false);
}

HudConsole::~HudConsole() {}

void HudConsole::setVisible(bool is_visible, bool animate) {
	HudWidget::setVisible(is_visible, animate);
	m_edit_box->setInputFocus(is_visible);
}

bool HudConsole::onInput(const InputEvent &event) {
	if(event.keyDown('`') || (event.mouseButtonDown(InputButton::left) && !isMouseOver(event))) {
		setVisible(false);
		return true;
	}
	if(isVisible() && event.key() >= 32 && event.key() <= 127)
		return true;

	//TODO: fixme

	return false;
}

bool HudConsole::onEvent(const HudEvent &event) {
	if(event.type == HudEvent::text_modified && event.source == m_edit_box.get()) {
		if(m_edit_box->text().empty()) {
			setVisible(false);
		} else {
			m_commands.push_back(m_edit_box->text());
			m_edit_box->setText("");
			m_edit_box->setInputFocus(true);
		}
		return true;
	}

	return false;
}

void HudConsole::onUpdate(double time_diff) {
	HudLayer::onUpdate(time_diff);

	if(!m_is_visible)
		m_edit_box->setText("");
}

void HudConsole::onDraw(Renderer2D &out) const { HudLayer::onDraw(out); }

const string HudConsole::getCommand() {
	string out;
	if(!m_commands.empty()) {
		out = m_commands.front();
		m_commands.erase(m_commands.begin());
	}
	return out;
}

}
