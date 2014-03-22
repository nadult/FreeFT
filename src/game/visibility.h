/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_VISIBILITY_H
#define GAME_VISIBILITY_H

#include "game/world.h"
#include "game/actor.h"

namespace game {

	class WorldVisInfo {
	public:
		WorldVisInfo();

		struct EntityVisInfo {
			EntityRef ref;
			double vis_time; // > 0: time visible, < 0: time invisible
		};



	protected:
		vector<EntityVisInfo> m_infos;
	};

}

#endif
