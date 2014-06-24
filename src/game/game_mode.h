/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_GAME_MODE_H
#define GAME_GAME_MODE_H

#include "game/base.h"
#include "game/inventory.h"

namespace game {

	class GameMode {
	public:
		GameMode(World &world) :m_world(world) { }
		virtual ~GameMode() { }

		GameMode(const GameMode&) = delete;
		void operator=(const GameMode&) = delete;

		// TODO: better name
		virtual GameModeId::Type modeId() const = 0;

		virtual void tick(double time_diff) = 0;
		virtual void onMessage(Stream&, int source_id) { }

		static GameMode *create(GameModeId::Type);

		bool isClient() const;
		bool isServer() const;

	protected:
		World &m_world;
	};

}

#endif

