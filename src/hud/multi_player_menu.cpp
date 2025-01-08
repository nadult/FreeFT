// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/multi_player_menu.h"

#include "gfx/drawing.h"
#include "hud/edit_box.h"
#include "hud/grid.h"
#include "net/client.h"
#include "sys/config.h"

#include <fwk/gfx/canvas_2d.h>
#include <fwk/gfx/font.h>

using namespace net;

//#define GEN_RANDOM

namespace hud {

namespace {

	const float2 s_button_size(17, 17);
	static const Color s_error_color =
		(Color)lerp((FColor)ColorId::red, (FColor)ColorId::white, 0.5f);

}

enum ButtonType {
	button_close,
	button_up,
	button_down,
	button_refresh,
	button_connect,
	button_password,
	button_nickname,
};

MultiPlayerMenu::MultiPlayerMenu(const FRect &rect, int2 window_size)
	: HudLayer(rect, HudLayer::slide_top), m_please_refresh(true), m_window_size(window_size) {
	int spacing = layer_spacing;

	setTitle("Connecting to server:");
	m_visible_time = 0.0f;

	m_waiting_for_refresh = false;
	m_waiting_to_connect = false;

	m_last_refresh_time = -1.0;
	m_last_connect_time = -1.0;

	FRect button_rect(s_button_size);
	button_rect += float2(rect.width() - spacing, rect.height() - spacing) - button_rect.size();

	PHudButton button_close = make_shared<HudClickButton>(button_rect);
	button_close->setIcon(HudIcon::close);

	button_rect -= float2(s_button_size.x + spacing * 2, 0.0f);
	PHudButton button_down = make_shared<HudClickButton>(button_rect);
	button_down->setIcon(HudIcon::down_arrow);
	button_down->setAccelerator(InputKey::pagedown);

	button_rect -= float2(s_button_size.x + spacing * 2, 0.0f);
	PHudButton button_up = make_shared<HudClickButton>(button_rect);
	button_up->setIcon(HudIcon::up_arrow);
	button_up->setAccelerator(InputKey::pageup);

	button_rect -= float2(s_button_size.x + spacing * 2, 0.0f);
	button_rect = button_rect.enlarge({50, 0}, {});
	PHudButton button_refresh = make_shared<HudClickButton>(button_rect);
	button_refresh->setLabel("refresh");

	button_rect -= float2(button_rect.width() + spacing * 2, 0.0f);
	button_rect = button_rect.enlarge({20, 0}, {});
	PHudButton button_connect = make_shared<HudClickButton>(button_rect);
	button_connect->setLabel("connect");
	button_connect->setAccelerator(InputKey::enter);

	button_rect = FRect(float2(185, s_button_size.y));
	button_rect += float2(rect.width() - button_rect.width() - spacing, spacing + topOffset());
	m_password = make_shared<HudEditBox>(button_rect, net::limits::max_password_size);
	m_password->setLabel("Pass: ");

	button_rect -= float2(button_rect.width() + spacing * 2, 0.0f);
	button_rect = {{button_rect.ex() - 230, button_rect.y()}, button_rect.max()};
	m_nick_name = make_shared<HudEditBox>(button_rect, net::limits::max_nick_name_size,
										  HudEditBox::mode_nick);
	m_nick_name->setLabel("Nick: ");
	m_nick_name->setText("random_dude");

	FRect sub_rect(rect.size());
	sub_rect = sub_rect.inset(float2(spacing, spacing * 2 + topOffset() + s_button_size.y),
							  float2(spacing, spacing * 2 + s_button_size.y));
	m_grid = make_shared<HudGrid>(sub_rect);

	m_grid->addColumn("Server name", 150.0f);
	m_grid->addColumn("Map name", 150.0f);
	m_grid->addColumn("Num players", 80.0f);
	m_grid->addColumn("Game mode", 80.0f);
	m_grid->addColumn("Ping", 50.0f);
	attach(m_grid);

	m_buttons.push_back(button_close);
	m_buttons.push_back(button_up);
	m_buttons.push_back(button_down);
	m_buttons.push_back(button_refresh);
	m_buttons.push_back(button_connect);
	m_buttons.push_back(m_password);
	m_buttons.push_back(m_nick_name);

	for(auto &button : m_buttons) {
		button->setButtonStyle(HudButtonStyle::small);
		attach(button);
	}

	m_client.reset(new net::Client());
}

MultiPlayerMenu::~MultiPlayerMenu() = default;

float MultiPlayerMenu::backAlpha() const { return alpha() * 0.6f; }

bool MultiPlayerMenu::onInput(const InputEvent &event) {
	if(event.keyDown(InputKey::esc)) {
		setVisible(false);
		return true;
	}

	return false;
}

bool MultiPlayerMenu::onEvent(const HudEvent &event) {
	if(event.type == HudEvent::button_clicked) {
		if(event.source == m_buttons[button_close]) {
			setVisible(false);
		}
		if(event.source == m_buttons[button_up])
			m_grid->scroll(-m_grid->numVisibleRows());
		if(event.source == m_buttons[button_down])
			m_grid->scroll(+m_grid->numVisibleRows());
		if(event.source == m_buttons[button_refresh]) {
			m_please_refresh = true;
		}
		if(event.source == m_buttons[button_connect] && m_client) {
			if(m_grid->selectedRow() != -1 && getTime() - m_last_connect_time > 1.0) {
				if(m_nick_name->text().size() < net::limits::min_nick_name_size) {
					setMessage(format("Nick name should have at least % characters",
									  (int)net::limits::min_nick_name_size),
							   s_error_color);
				} else {
					Address address = m_servers[m_grid->selectedRow()].address;
					if(address.isValid()) {
						m_last_connect_time = getTime();
						m_client->connect(address, m_nick_name->text(), m_password->text());
						m_waiting_to_connect = true;
					}
				}
			}
		}

		return true;
	}

	return false;
}

void MultiPlayerMenu::onUpdate(double time_diff) {
	HudLayer::onUpdate(time_diff);
	updateLobbyData();

	m_buttons[button_connect]->setGreyed(!m_client || m_grid->selectedRow() == -1);
	m_buttons[button_up]->setGreyed(m_grid->scrollPos() == 0);
	m_buttons[button_down]->setGreyed(m_grid->scrollPos() + m_grid->numVisibleRows() >=
									  m_grid->numRows());

	if(m_waiting_to_connect && m_client) {
		if(m_client->mode() == Client::Mode::waiting_for_world_update) {
			m_waiting_to_connect = false;
		} else if(m_client->mode() == Client::Mode::refused) {
			setMessage(format("Connection refused: %", describe(m_client->refuseReason())),
					   s_error_color);
			m_waiting_to_connect = false;
		} else if(getTime() - m_last_connect_time > 5.0) {
			setMessage("Error while connecting to server...", s_error_color);
			m_waiting_to_connect = false;
		}
	}
}

void MultiPlayerMenu::updateLobbyData() {
	if(!m_client)
		return;

#ifdef GEN_RANDOM
	if(!m_please_refresh)
		return;
	m_please_refresh = false;
	m_servers.clear();

	vector<const char *> snames = {"Maxx server", "Maxx server", "Poly killers", "Hello kitty fans",
								   "Duck hunters"};
	vector<const char *> mnames = {"City carnage", "Bunker assault", "Motor sports", "Crypt #13",
								   "Desert"};

	for(int n = 0; n < 30; n++) {
		ServerInfo info;
		info.game_mode = (game::GameModeId)(rand() % game::GameModeId::count);
		info.max_players = rand() % 2 ? rand() % 2 ? 4 : 8 : 16;
		info.num_players = rand() % info.max_players + 1;
		info.ping = rand() % 2 ? rand() % 200 : rand() % 500;
		info.map_name = mnames[rand() % mnames.size()];
		info.server_name = format("% #%", snames[rand() % snames.size()], rand() % 4 + 1);
		m_servers.push_back(info);
	}

	updateGrid();
	return;
#endif

	if(m_please_refresh && getTime() - m_last_refresh_time > 1.0) {
		m_client->requestLobbyData();
		m_last_refresh_time = getTime();
		m_please_refresh = false;
		m_waiting_for_refresh = true;
	}

	m_client->beginFrame();
	m_client->finishFrame();

	vector<ServerStatusChunk> new_data;
	if(m_client->getLobbyData(new_data)) {
		m_waiting_for_refresh = false;

		//TODO: support for data send in multiple packets
		if(new_data.empty()) {
			setMessage("No servers active", ColorId::white);
			m_servers.clear();
		}

		int index = m_grid->selectedRow();
		Address sel_address = m_servers.inRange(index) ? m_servers[index].address : Address();

		m_servers.clear();
		for(int n = 0; n < (int)new_data.size(); n++) {
			ServerInfo new_info;
			((net::ServerStatusChunk &)new_info) = new_data[n];
			new_info.ping = 0;
			m_servers.push_back(new_info);
		}

		updateGrid();
		if(sel_address.isValid()) {
			for(int n = 0; n < (int)m_servers.size(); n++)
				if(m_servers[n].address == sel_address)
					m_grid->selectRow(n);
		}
	}

	if(m_waiting_for_refresh && getTime() - m_last_refresh_time > 5.0) {
		setMessage("No data received from lobby server...", s_error_color);
		m_waiting_for_refresh = false;
	}
}

void MultiPlayerMenu::updateGrid() {
	m_grid->clearRows();
	for(int n = 0; n < (int)m_servers.size(); n++) {
		m_grid->addRow(n);
		for(int c = 0; c < m_grid->numColumns(); c++)
			m_grid->setCell(n, c, cellText(n, c));
	}
}

void MultiPlayerMenu::onDraw(Canvas2D &out) const {
	FRect back_quad((float2)m_window_size);

	out.addFilledRect(back_quad, mulAlpha(ColorId::black, m_visible_time * 0.8f));
	HudLayer::onDraw(out);

	if(!m_message.empty()) {
		double msg_time = getTime() - m_message_time;
		double alpha = min(1.0, 5.0 - msg_time);

		if(msg_time < 5.0) {
			FontStyle style{(IColor)mulAlpha(m_message_color, alpha),
							(IColor)mulAlpha(ColorId::black, alpha), HAlign::left, VAlign::bottom};
			m_font->draw(out, rect() + float2(spacing, 0.0f), style, m_message);
		}
	}
}

const string MultiPlayerMenu::cellText(int server_id, int col_id) const {
	DASSERT(server_id >= 0 && server_id < (int)m_servers.size());
	const auto &row = m_servers[server_id];

	if(col_id == 0)
		return row.server_name;
	else if(col_id == 1)
		return row.map_name;
	else if(col_id == 2)
		return format("% / %", row.num_players, row.max_players);
	else if(col_id == 3)
		return toString(row.game_mode);
	else if(col_id == 4)
		return format("%", row.ping);

	return string();
}

void MultiPlayerMenu::setMessage(const string &message, Color color) {
	m_message = message;
	m_message_time = getTime();
	m_message_color = color;
}

bool MultiPlayerMenu::isClientReady() const {
	return m_client && m_client->mode() == Client::Mode::waiting_for_world_update;
}

net::PClient &&MultiPlayerMenu::getClient() { return std::move(m_client); }

}
