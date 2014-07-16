/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef IO_CONTROLLER_H
#define IO_CONTROLLER_H

#include "game/world.h"
#include "game/visibility.h"

namespace hud {
	class Hud;
	class HudConsole;
}

namespace io {

	class Controller {
	public:
		Controller(const int2 &resolution, game::PWorld world, bool show_stats);
		~Controller();

		void updatePC();
		void update(double time_diff);
		void updateView(double time_diff);
		void onInput(const InputEvent&);
		void draw();

		void drawVisibility(game::EntityRef);

	protected:
		void sendOrder(game::POrder&&);

		game::Actor *getActor();

		//TODO: m_world can be a reference, not a pointer
		game::PWorld m_world;
		game::WorldViewer m_viewer;
		game::GameMode *m_game_mode;
		game::PPlayableCharacter m_pc;
		game::EntityRef m_actor_ref;
		Ptr<game::PCController> m_pc_controller;

		Ptr<hud::HudConsole> m_console;
		Ptr<hud::Hud> m_hud;

		int2 m_resolution, m_view_pos;

		string m_profiler_stats;
		double m_last_time, m_stats_update_time;
		bool m_show_stats;

		Ray m_screen_ray;
		game::Intersection m_isect, m_full_isect;
		float3 m_target_pos;
		float3 m_last_look_at;

		game::Path m_last_path;
	};

	typedef unique_ptr<Controller> PController;

}

#endif
