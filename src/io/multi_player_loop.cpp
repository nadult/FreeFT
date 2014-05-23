/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/multi_player_loop.h"
#include "gfx/device.h"
#include "net/client.h"
#include "sys/config.h"
#include "audio/device.h"

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
	};
		
	MultiPlayerMenu::MultiPlayerMenu(const FRect &rect, HudStyle style) :HudLayer(rect) {
		m_slide_left = false;
		m_visible_time = 0.0f;

		style.border_offset *= 0.5f;
		setStyle(style);

		FRect button_rect(s_button_size);
		button_rect += float2(rect.width() - spacing, rect.height() - spacing) - button_rect.size();

		PHudWidget button_close = new HudWidget(button_rect);
		button_close->setIcon(HudIcon::close);

		button_rect -= float2(s_button_size.x + HudWidget::spacing, 0.0f);
		PHudWidget button_up = new HudWidget(button_rect);
		button_up->setIcon(HudIcon::up_arrow);

		button_rect -= float2(s_button_size.x + HudWidget::spacing, 0.0f);
		PHudWidget button_down = new HudWidget(button_rect);
		button_down->setIcon(HudIcon::down_arrow);

		button_rect -= float2(s_button_size.x + HudWidget::spacing, 0.0f);
		button_rect.min.x -= 50.0f;
		PHudWidget button_refresh = new HudWidget(button_rect);
		button_refresh->setText("refresh");

		m_buttons.push_back(button_close);
		m_buttons.push_back(button_up);
		m_buttons.push_back(button_down);
		m_buttons.push_back(button_refresh);

		attach(button_close.get());
		attach(button_up.get());
		attach(button_down.get());
		attach(button_refresh.get());

		m_columns.emplace_back(Column{ ColumnType::server_name,	"Server name",	150.0f });
		m_columns.emplace_back(Column{ ColumnType::map_name,	"Map name",		150.0f });
		m_columns.emplace_back(Column{ ColumnType::num_players,	"Num players",	80.0f });
		m_columns.emplace_back(Column{ ColumnType::game_mode,	"Game mode",	80.0f });
		m_columns.emplace_back(Column{ ColumnType::ping,		"Ping",			50.0f });
	}
	
	float MultiPlayerMenu::backAlpha() const {
		return 0.6f;
	}
		
	void MultiPlayerMenu::updateColumnRects() {
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
			pos += isize + spacing;
		}
	}
		
	void MultiPlayerMenu::update(bool is_active, double time_diff) {
		HudLayer::update(is_active, time_diff);
		float2 mouse_pos = float2(gfx::getMousePos()) - rect().min;

		updateColumnRects();

		if(is_active && m_is_visible) {
			for(int n = 0; n < (int)m_buttons.size(); n++)
				m_buttons[n]->setFocus(gfx::isMouseKeyPressed(0) && m_buttons[n]->rect().isInside(mouse_pos));

			if(m_buttons[button_close]->isPressed(mouse_pos)) {
				audio::playSound("butn_text", 1.0f);
				setVisible(false);
			}
			if(m_buttons[button_up]->isPressed(mouse_pos)) {
				audio::playSound("butn_text", 1.0f);
			}
			if(m_buttons[button_down]->isPressed(mouse_pos)) {
				audio::playSound("butn_text", 1.0f);
			}
			if(m_buttons[button_refresh]->isPressed(mouse_pos)) {
				audio::playSound("butn_text", 1.0f);
			}
		}
	}

	void MultiPlayerMenu::draw() const {
		FRect back_quad(gfx::getWindowSize());

		DTexture::unbind();
		drawQuad(back_quad, mulAlpha(Color::black, m_visible_time * 0.8f));

		HudLayer::draw();

		DTexture::unbind();
		for(int n = 0; n < (int)m_columns.size(); n++) {
			const Column &column = m_columns[n];
			drawQuad(column.rect, mulAlpha(lerp(Color::white, Color::green, n & 1? 0.3f : 0.6f), 0.5f));
		}
	}

}

namespace io {

	static void parseServerList(std::map<Address, net::ServerStatusChunk> &servers, InPacket &packet) {
		while(packet.pos() < packet.size()) {
			ServerStatusChunk chunk;
			packet >> chunk;

			if(chunk.address.isValid())
				servers[chunk.address] = chunk;
		}
	}

	static Address findServer(int local_port) {
		using namespace net;

		Address local_addr((u16)local_port);
		Socket socket(local_addr); //TODO: use socket from Client class
		Address lobby_address = lobbyServerAddress();

		OutPacket request(0, -1, -1, PacketInfo::flag_lobby);
		request << LobbyChunkId::server_list_request;
		socket.send(request, lobby_address);

		double start_time = getTime();

		while(getTime() - start_time < 5.0) {
			InPacket packet;
			Address source;
			int ret = socket.receive(packet, source);
			if(ret <= 0) {
				sleep(0.01);
				continue;
			}

			try {
				LobbyChunkId::Type chunk_id;
				packet >> chunk_id;

				if(chunk_id == LobbyChunkId::server_list) {
					std::map<Address, ServerStatusChunk> servers;
					parseServerList(servers, packet);

					if(servers.empty()) {
						printf("No servers currently active\n");
						return Address();
					}
					else {
						auto it = servers.begin();
						//TODO: punch through
						return it->second.address;
					}
				}
			}
			catch(...) {
				continue;
			}


		}
				
		printf("Timeout when connecting to lobby server\n");
		return Address();
	}

	net::PClient createClient(const string &server_name, int port) {
		Address server_address = findServer(port);
		//Address server_address = server_name.empty()? findServer(port) : Address(resolveName(server_name.c_str()), server_port);

		if(!server_address.isValid())
			THROW("Invalid server address\n");

		net::PClient client(new net::Client(port));
		client->connect(server_address);
		
		while(client->mode() != Client::Mode::connected) {
			client->beginFrame();
			client->finishFrame();
			sleep(0.05);
		}

		return client;
	}

	MultiPlayerLoop::MultiPlayerLoop(net::PClient client) {
		DASSERT(client && client->mode() == Client::Mode::connected);
		m_client = std::move(client);
		m_world = m_client->world();

		Config config = loadConfig("client");

		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, m_client->actorRef(), config.profiler_enabled));
	}

	bool MultiPlayerLoop::tick(double time_diff) {
		using namespace gfx;

		m_controller->update(time_diff);
		m_client->beginFrame();

		m_world->simulate(time_diff);
		m_client->finishFrame();
		m_controller->updateView(time_diff);

		m_controller->draw();
		return !isKeyPressed(Key_esc);
	}

}
