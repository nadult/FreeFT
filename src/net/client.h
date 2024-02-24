// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"
#include "game/world.h"
#include "net/base.h"
#include "net/host.h"

namespace net {

class Client : public net::LocalHost, public game::Replicator {
  public:
	enum class Mode {
		disconnected,
		connecting,
		refused,
		timeout,

		waiting_for_world_update,
		world_updated,
		playing,
	};

	static constexpr int timeout = 5;

	Client(int port = 0);
	~Client();

	bool getLobbyData(vector<ServerStatusChunk> &out);
	void requestLobbyData();

	void connect(Address address, const string &nick_name, const string &password);
	void disconnect();

	Mode mode() const { return m_mode; }

	void beginFrame();
	void finishFrame();

	const LevelInfoChunk &levelInfo() const { return m_level_info; }
	void setWorld(game::PWorld);
	bool needWorldUpdate() const { return m_mode == Mode::waiting_for_world_update; }

	game::PWorld world() { return m_world; }

	RefuseReason refuseReason() const {
		DASSERT(m_mode == Mode::refused);
		return m_refuse_reason;
	}

  protected:
	void entityUpdate(InChunk &chunk);
	void sendMessage(CSpan<char>, int target_id) override;

  private:
	LevelInfoChunk m_level_info;
	game::PWorld m_world;

	string m_nick_name;
	Address m_server_address;
	int m_server_id, m_client_id;
	RefuseReason m_refuse_reason;
	Mode m_mode;
};

}
