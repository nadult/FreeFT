// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef IO_CONTROLLER_H
#define IO_CONTROLLER_H

#include "game/world.h"
#include "game/visibility.h"
#include "game/game_mode.h"
#include <fwk_input.h>

namespace hud {
	class Hud;
	class HudConsole;
	class HudTargetInfo;
}

namespace io {

	class Controller {
	public:

		Controller(game::PWorld world, bool debug_info);
		~Controller();

		void updateView(double time_diff);
		void update(double time_diff);
		void draw() const;

		int exitRequested() const { return m_is_exiting; }

		double timeMultiplier() const { return m_time_multiplier; }
		void setTimeMultiplier(double mul) { m_time_multiplier = mul; }
		static void setProfilerStats(string);

	protected:
		void updatePC();
		void onInput(const InputEvent&);
		void drawDebugInfo(Renderer2D&) const;

		void sendOrder(game::POrder&&);

		game::Actor *getActor();

		struct ShownMessage {
			ShownMessage() :anim_time(0.0f) { }
			ShownMessage(game::UserMessage msg) :message(msg), anim_time(0.0f) { }

			bool empty() const { return message.text.empty(); }
			const string text() const { return message.text; }

			game::UserMessage message;
			float anim_time;
		};

		//TODO: m_world can be a reference, not a pointer
		game::PWorld m_world;
		game::WorldViewer m_viewer;
		game::GameMode *m_game_mode;
		game::PPlayableCharacter m_pc;
		game::EntityRef m_actor_ref;
		shared_ptr<game::PCController> m_pc_controller;

		shared_ptr<hud::HudConsole> m_console;
		shared_ptr<hud::Hud> m_hud;
		shared_ptr<hud::HudTargetInfo> m_target_info;

		int2 m_view_pos;
		int2 m_last_mouse_pos;

		double m_time_multiplier;
		bool m_debug_ai, m_debug_navi;
		bool m_show_debug_info;

		Ray3F m_screen_ray;
		game::Intersection m_isect, m_full_isect;
		float3 m_target_pos, m_last_look_at;
		game::Path m_last_path;

		ShownMessage m_main_message;

		int m_is_exiting;
	};

}

#endif
