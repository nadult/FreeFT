/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/multi_player_loop.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "net/client.h"
#include "sys/config.h"

using namespace net;
using namespace gfx;

namespace hud {

	namespace {

		const float2 s_button_size(15, 15);

	}

	enum ButtonType {
		button_close,
		button_up,
		button_down,
		button_refresh,
		button_connect,
	};
		
	MultiPlayerMenu::MultiPlayerMenu(const FRect &rect, HudStyle style) :HudLayer(rect),
	  m_max_visible_rows(0), m_row_offset(0), m_selection(-1), m_please_refresh(true) {
		m_slide_left = false;
		m_visible_time = 0.0f;

		m_waiting_for_refresh = false;
		m_waiting_to_connect = false;

		m_last_refresh_time = -1.0;
		m_last_connect_time = -1.0;

		style.border_offset *= 0.5f;
		setStyle(style);

		FRect button_rect(s_button_size);
		button_rect += float2(rect.width() - spacing, rect.height() - spacing) - button_rect.size();

		PHudButton button_close = new HudButton(button_rect);
		button_close->setIcon(HudIcon::close);

		button_rect -= float2(s_button_size.x + HudButton::spacing, 0.0f);
		PHudButton button_down = new HudButton(button_rect);
		button_down->setIcon(HudIcon::down_arrow);
		button_down->setAccelerator(Key::pagedown);

		button_rect -= float2(s_button_size.x + HudButton::spacing, 0.0f);
		PHudButton button_up = new HudButton(button_rect);
		button_up->setIcon(HudIcon::up_arrow);
		button_up->setAccelerator(Key::pageup);

		button_rect -= float2(s_button_size.x + HudButton::spacing, 0.0f);
		button_rect.min.x -= 50.0f;
		PHudButton button_refresh = new HudButton(button_rect);
		button_refresh->setText("refresh");
		
		button_rect -= float2(button_rect.width() + HudButton::spacing, 0.0f);
		button_rect.min.x -= 20.0f;
		PHudButton button_connect = new HudButton(button_rect);
		button_connect->setText("connect");
		button_connect->setAccelerator(Key::enter);

		m_buttons.push_back(button_close);
		m_buttons.push_back(button_up);
		m_buttons.push_back(button_down);
		m_buttons.push_back(button_refresh);
		m_buttons.push_back(button_connect);

		attach(button_close.get());
		attach(button_up.get());
		attach(button_down.get());
		attach(button_refresh.get());
		attach(button_connect.get());

		m_columns.emplace_back(Column{ ColumnType::server_name,	"Server name",	150.0f });
		m_columns.emplace_back(Column{ ColumnType::map_name,	"Map name",		150.0f });
		m_columns.emplace_back(Column{ ColumnType::num_players,	"Num players",	80.0f });
		m_columns.emplace_back(Column{ ColumnType::game_mode,	"Game mode",	80.0f });
		m_columns.emplace_back(Column{ ColumnType::ping,		"Ping",			50.0f });

		m_client.reset(new net::Client());
	}
	
	float MultiPlayerMenu::backAlpha() const {
		return 0.6f;
	}
		
	void MultiPlayerMenu::updateRects() {
		FRect sub_rect = rect();
		sub_rect.min += float2(spacing, spacing);
		sub_rect.max -= float2(spacing, spacing * 2 + s_button_size.y);
		
		float size_left = sub_rect.width(), min_sum = 0.0f;
		for(int n = 0; n < (int)m_columns.size(); n++) {
			size_left -= m_columns[n].min_size + spacing;
			min_sum += m_columns[n].min_size;
		}

		float frac = 0.0f, pos = 0.0f;
		for(int n = 0; n < (int)m_columns.size(); n++) {
			float col_size = m_columns[n].min_size + size_left * (m_columns[n].min_size / min_sum) + frac;
			int isize = (int)col_size;
			frac = col_size - isize;
			m_columns[n].rect = FRect(sub_rect.min.x + pos, sub_rect.min.y, sub_rect.min.x + pos + isize, sub_rect.max.y);
			if(n == (int)m_columns.size() - 1)
				m_columns[n].rect.max.x = sub_rect.max.x;
			pos += isize + spacing;
		}

		float row_size = m_font->lineHeight();
		m_max_visible_rows = (int)((sub_rect.height() + spacing) / (row_size + spacing)) - 1;

		pos = sub_rect.min.y + row_size + spacing;
		for(int r = 0; r < (int)m_servers.size(); r++) {
			if(r < m_row_offset || r >= m_row_offset + m_max_visible_rows) {
				m_servers[r].rect = FRect();
				continue;
			}

			m_servers[r].rect = FRect(sub_rect.min.x, pos, sub_rect.max.x, pos + row_size);
			pos += row_size + spacing;
		}
	}
		
	void MultiPlayerMenu::onUpdate(double time_diff) {
/*		is_active &= m_is_visible && m_client;
		HudLayer::update(is_active, time_diff);
		float2 mouse_pos((float2)gfx::getMousePos() - rect().min);

		updateLobbyData();

		if(isKeyDown(Key::esc))
			setVisible(false);

		for(int n = 0; n < (int)m_buttons.size(); n++)
			m_buttons[n]->setFocus(gfx::isMouseKeyPressed(0) && m_buttons[n]->rect().isInside(mouse_pos) && is_active);

		m_buttons[button_connect]->setVisible(m_selection != -1);
		m_buttons[button_up]->setVisible(m_row_offset > 0);
		m_buttons[button_down]->setVisible(m_row_offset + m_max_visible_rows < (int)m_servers.size());

		if(is_active) {
			if(m_buttons[button_close]->isPressed(mouse_pos)) {
				playSound(HudSound::button);
				setVisible(false);
			}
			if(m_buttons[button_up]->isPressed(mouse_pos)) {
				m_row_offset -= m_max_visible_rows;
				m_row_offset = max(m_row_offset, 0);
				playSound(HudSound::button);
			}
			if(m_buttons[button_down]->isPressed(mouse_pos)) {
				if(m_row_offset + m_max_visible_rows < (int)m_servers.size())
					m_row_offset += m_max_visible_rows;
				playSound(HudSound::button);
			}
			if(m_buttons[button_refresh]->isPressed(mouse_pos)) {
				playSound(HudSound::button);
				m_please_refresh = true;
			}
			if(m_buttons[button_connect]->isPressed(mouse_pos)) {
				if(m_selection != -1 && getTime() - m_last_connect_time > 1.0) {
					Address address = m_servers[m_selection].address;
					if(address.isValid()) {
						m_last_connect_time = getTime();
						m_client->connect(address, "bubba", "");
						playSound(HudSound::button);
						m_waiting_to_connect = true;
					}
				}
			}
		}

		if(m_waiting_to_connect && getTime() - m_last_connect_time > 5.0) {
			setMessage("Error while connecting to server...", lerp(Color::white, Color::red, 0.5f));
			m_waiting_to_connect = false;
		}

		if(m_selection < m_row_offset || m_selection >= m_row_offset + m_max_visible_rows)
			m_selection = -1;

		updateRects();

		mouse_pos += rect().min;
		for(int n = 0; n < (int)m_servers.size(); n++) {
			ServerInfo &info = m_servers[n];
			FRect rect = info.rect;
			rect.min.y -= spacing * 0.5f;
			rect.max.y += spacing * 0.5f;

			info.is_mouse_over = rect.isInside(mouse_pos);
			if(info.is_mouse_over && isMouseKeyDown(0))
				m_selection = n;
			animateValue(info.over_time, time_diff * 5.0f, info.is_mouse_over);
			animateValue(info.selection_time, time_diff * 5.0f, n == m_selection);
			info.over_time = max(info.over_time, info.selection_time);
		}*/
	}

	void MultiPlayerMenu::updateLobbyData() {
		if(!m_client)
			return;

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
				setMessage("No servers active", Color::white);
				m_servers.clear();
			}

			auto old_servers = m_servers;
			m_servers.clear();
			for(int n = 0; n < (int)new_data.size(); n++) {
				ServerInfo new_info;
				((net::ServerStatusChunk&)new_info) = new_data[n];
				new_info.ping = 0;
				new_info.is_mouse_over = false;
				new_info.over_time = 0.0f;
				new_info.selection_time = 0.0f;
				m_servers.push_back(new_info);
			}
			m_row_offset = 0;
		}

		if(m_waiting_for_refresh && getTime() - m_last_refresh_time > 5.0) {
			setMessage("No data received from lobby server...", lerp(Color::white, Color::red, 0.5f));
			m_waiting_for_refresh = false;
		}

#ifdef GEN_RANDOM
		m_servers.clear();

		vector<const char*> snames = { "Maxx server", "Maxx server", "Poly killers", "Hello kitty fans", "Duck hunters" };
		vector<const char*> mnames = { "City carnage", "Bunker assault", "Motor sports", "Crypt #13", "Desert" };

		for(int n = 0; n < 30; n++) {
			ServerInfo info;
			info.game_mode = (game::GameMode::Type)(rand() % game::GameMode::count);
			info.max_players = rand() % 2? rand() % 2? 4 : 8 : 16;
			info.num_players = rand() % info.max_players + 1;
			info.ping = rand() % 2? rand() % 200 : rand() % 500;
			info.map_name = mnames[rand() % mnames.size()];
			info.server_name = format("%s #%d", snames[rand() % snames.size()], rand() % 4 + 1);
			info.over_time = 0.0f;
			info.selection_time = 0.0f;
			m_servers.push_back(info);
		}
		m_row_offset = 0;
#endif
	}

	void MultiPlayerMenu::onDraw() const {
		FRect back_quad(gfx::getWindowSize());

		DTexture::unbind();
		drawQuad(back_quad, mulAlpha(Color::black, m_visible_time * 0.8f));

		HudLayer::onDraw();

		for(int n = 0; n < (int)m_columns.size(); n++) {
			const Column &column = m_columns[n];
			FRect rect = column.rect;

			DTexture::unbind();
			drawQuad(rect, mulAlpha(lerp(Color::white, Color::green, n & 1? 0.3f : 0.6f), 0.5f));
			m_font->draw(rect, {Color::white, Color::black, HAlign::center, VAlign::top}, column.title);
			
			for(int r = m_row_offset, rend = min((int)m_servers.size(), m_row_offset + m_max_visible_rows); r < rend; r++) {
				const ServerInfo &info = m_servers[r];
				FRect row_rect = info.rect;
				m_font->draw(FRect(rect.min.x, row_rect.min.y, rect.max.x, row_rect.max.y),
							 {lerp(Color(200, 200, 200), Color::white, info.over_time), Color::black, HAlign::center},
							 cellText(r, column.type));

				if(m_selection == r && info.selection_time > 0.0f)
					drawBorder(info.rect, mulAlpha(Color(200, 255, 200, 80), info.selection_time * info.selection_time),
							float2(50.0f, 50.0f) * (1.0f - info.selection_time), 500.0f);
			}
		}

		if(!m_message.empty()) {
			double msg_time = getTime() - m_message_time;
			double alpha = min(1.0, 5.0 - msg_time);

			if(msg_time < 5.0)
				m_font->draw(rect() + float2(spacing, 0.0f),
						     {mulAlpha(m_message_color, alpha), mulAlpha(Color::black, alpha), HAlign::left, VAlign::bottom}, m_message);
		}
	}
		
	const string MultiPlayerMenu::cellText(int server_id, ColumnType col_type) const {
		DASSERT(server_id >= 0 && server_id < (int)m_servers.size());
		const auto &row = m_servers[server_id];

		if(col_type == ColumnType::server_name)
			return row.server_name;
		else if(col_type == ColumnType::map_name)
			return row.map_name;
		else if(col_type == ColumnType::num_players)
			return format("%d / %d", row.num_players, row.max_players);
		else if(col_type == ColumnType::game_mode)
			return game::GameModeId::toString(row.game_mode);
		else if(col_type == ColumnType::ping)
			return format("%d", row.ping);

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
		
	net::PClient &&MultiPlayerMenu::getClient() {
		return std::move(m_client);
	}

}

namespace io {

	MultiPlayerLoop::MultiPlayerLoop(net::PClient client) {
		DASSERT(client && client->world());
		m_client = std::move(client);
		m_world = m_client->world();

		Config config = loadConfig("client");

		//TODO: wait until initial entity information is loaded?
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, config.profiler_enabled));
	}

	bool MultiPlayerLoop::tick(double time_diff) {
		using namespace gfx;

		//TODO: handle change of map
		m_controller->update(time_diff);
		m_client->beginFrame();

		m_world->simulate(time_diff);
		m_client->finishFrame();
		m_controller->updateView(time_diff);

		m_controller->draw();
		return !isKeyPressed(Key::esc);
	}

}
