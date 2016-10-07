/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/controller.h"

#include "game/actor.h"
#include "game/item.h"
#include "game/visibility.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"
#include "game/all_orders.h"
#include "game/brain.h"

#include "gfx/scene_renderer.h"
#include "audio/device.h"

#include "hud/console.h"
#include "hud/hud.h"
#include "hud/target_info.h"

using namespace game;

namespace io {

	static const float s_exit_anim_length = 0.7f;
	static const float2 target_info_size(200.0f, 80.0f);
	
	static string s_profiler_stats;

	void Controller::setProfilerStats(string stats) {
		s_profiler_stats = std::move(stats);
	}

	Controller::Controller(PWorld world, bool debug_info)
		: m_world(world), m_viewer(world),  m_view_pos(0, 0),
		  m_show_debug_info(debug_info), m_debug_navi(false), m_debug_ai(false), m_is_exiting(0),
		  m_time_multiplier(1.0) {
		auto resolution = GfxDevice::instance().windowSize();

		DASSERT(world);
		m_console = make_shared<hud::HudConsole>(resolution);
		m_hud = make_shared<hud::Hud>(world, resolution);
		m_target_info = make_shared<hud::HudTargetInfo>(FRect(target_info_size));
		m_target_info->setVisible(false, false);

		m_game_mode = m_world->gameMode();
		ASSERT(m_game_mode);
		updatePC();

		if(const Actor *actor = getActor())
			m_view_pos = int2(worldToScreen(actor->pos())) - resolution / 2;

		m_last_look_at = float3(0, 0, 0);

		if(m_world->isClient())
			m_hud->showLayer(hud::layer_character);
	}

	Controller::~Controller() {}

	// TODO: better name
	void Controller::updatePC() {
		const GameClient *client = m_game_mode->currentClient();
		const vector<PlayableCharacter> &pcs = client ? client->pcs : vector<PlayableCharacter>();
		PPlayableCharacter pc;

		if((pcs.empty() && !m_pc) || (m_pc && pcs[0] == *m_pc))
			return;

		m_pc = make_shared<PlayableCharacter>(pcs[0]);
		m_pc_controller = m_pc ? make_shared<PCController>(*m_world, *m_pc) : nullptr;

		m_actor_ref = m_pc ? m_pc->entityRef() : EntityRef();
		m_viewer.setSpectator(m_actor_ref);
		m_hud->setPCController(m_pc_controller);
	}

	Actor *Controller::getActor() {
		Actor *actor = nullptr;
		if(m_pc)
			actor = m_world->refEntity<Actor>(m_pc->entityRef());
		return actor;
	}

	void Controller::sendOrder(game::POrder &&order) {
		if(m_pc && m_world)
			m_world->sendOrder(std::move(order), m_pc->entityRef());
	}

	void Controller::onInput(const InputEvent &event) {
		if(m_console->handleInput(event) || m_hud->handleInput(event))
			return;

		if((event.mouseButtonPressed(InputButton::left) &&
			event.hasModifier(InputEvent::mod_lctrl)) ||
		   event.mouseButtonPressed(InputButton::middle))
			m_view_pos -= event.mouseMove();
		if(event.isMouseEvent())
			m_last_mouse_pos = event.mousePos();

		bool mouse_over_hud = m_console->isMouseOver(event) || m_hud->isMouseOver(event);

		Actor *actor = getActor();
		if(actor && actor->isDead())
			actor = nullptr;

		if(event.keyDown('`'))
			m_console->setVisible(m_console->isVisible() ^ 1);
		else if(event.keyDown('h'))
			m_hud->setVisible(m_hud->isVisible() ^ 1);
		else if(event.keyDown('i'))
			m_hud->showLayer(hud::layer_inventory);
		else if(event.keyDown('c'))
			m_hud->showLayer(hud::layer_character);
		else if(event.keyDown('o'))
			m_hud->showLayer(hud::layer_options);
		else if(event.keyDown('v'))
			m_hud->showLayer(hud::layer_class);
		else if(event.keyDown('s'))
			m_hud->showLayer(hud::layer_stats);
		else if(event.keyDown('q'))
			m_pc_controller->setStance(Stance::stand);
		else if(event.keyDown('a'))
			m_pc_controller->setStance(Stance::crouch);
		else if(event.keyDown('z'))
			m_pc_controller->setStance(Stance::prone);
		else if(event.keyDown('r'))
			m_pc_controller->reload();

		if(event.isMouseOverEvent() && mouse_over_hud) {
			m_isect = m_full_isect = Intersection();
		} else if(event.isMouseOverEvent() && !mouse_over_hud) {
			m_screen_ray = screenRay((int2)event.mousePos() + m_view_pos);

			FlagsType flags =
				Flags::walkable_tile |
				(Flags::entity & ~(Flags::projectile | Flags::impact | Flags::trigger));
			m_isect =
				m_viewer.pixelIntersect((int2)event.mousePos() + m_view_pos, {flags, m_actor_ref});
			Segment screen_ray(m_screen_ray.origin(), m_screen_ray.at(1024.0f));
			if(m_isect.empty() || m_isect.isTile())
				m_isect = m_viewer.trace(screen_ray, {flags, m_actor_ref});

			// TODO: pixel intersect may find an intersection, but the ray doesn't necessarily
			// has to intersect bounding box of the object
			m_full_isect =
				m_viewer.pixelIntersect((int2)event.mousePos() + m_view_pos, m_actor_ref);
			if(m_full_isect.empty())
				m_full_isect = m_viewer.trace(screen_ray, m_actor_ref);

			if(!m_full_isect.empty() && actor) {
				// TODO: send it only, when no other order is in progress (or has been sent and
				// wasn't
				// finished)
				if(m_full_isect.distance() < constant::inf &&
				   m_full_isect.distance() > -constant::inf) {
					float3 look_at = m_screen_ray.at(m_full_isect.distance());
					if(look_at != m_last_look_at &&
					   distance(look_at.xz(), actor->boundingBox().center().xz()) > 5.0f) {
						m_world->sendOrder(new LookAtOrder(look_at), m_actor_ref);
						m_last_look_at = look_at;
					}
				}
			}
		} else if(event.mouseButtonDown(InputButton::left) &&
				  !event.hasModifier(InputEvent::mod_lctrl)) {
			Entity *entity = m_world->refEntity(m_isect);

			if(entity) {
				m_world->sendOrder(new InteractOrder(entity->ref()), m_actor_ref);
			} else if(m_isect.isTile()) {
				// TODO: pixel intersect always returns distance == 0
				int3 wpos = int3(m_screen_ray.at(m_isect.distance()) + float3(0, 0.5f, 0));

				bool run =
					actor &&
					!event.hasModifier(
						InputEvent::mod_lshift); // && distance(float3(wpos), actor->pos()) > 10.0f;
				m_world->sendOrder(new MoveOrder(wpos, run), m_actor_ref);
			}
		} else if(event.mouseButtonDown(InputButton::right) && actor) {
			Maybe<AttackMode> mode;
			if(event.hasModifier(InputEvent::mod_lshift)) {
				const Weapon &weapon = actor->inventory().weapon();
				if(weapon.proto().attack_modes & AttackModeFlags::burst)
					mode = AttackMode::burst;
				else if(weapon.canKick())
					mode = AttackMode::kick;
			}

			if(!m_isect.empty()) {
				Entity *entity = m_world->refEntity(m_isect);
				if(entity) {
					m_world->sendOrder(new AttackOrder(mode, entity->ref()), m_actor_ref);
				} else
					m_world->sendOrder(new AttackOrder(mode, m_screen_ray.at(m_isect.distance())),
									   m_actor_ref);
			}
		} else if(event.keyDown('T') && !m_isect.empty() && actor) {
			float3 pos = m_screen_ray.at(m_isect.distance());
			actor->setPos((float3)int3(pos + float3(0.5f, 0.5f, 0.5f)));
			actor->fixPosition();
		}
	}

	void Controller::update(double time_diff) {
		updatePC();

		auto &device = GfxDevice::instance();
		auto resolution = device.windowSize();
		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(actor)
			audio::setListener(actor->pos(), actor->estimateMove(1.0f),
							   normalize(float3(-1, 0, -1)));
		else {
			Ray mid_ray = screenRay(m_view_pos + resolution / 2);
			auto isect = m_world->trace(Segment(mid_ray.origin(), mid_ray.at(1024.0f)));
			if(isect) {
				float3 listener_pos = mid_ray.at(isect.distance());
				audio::setListener(listener_pos, float3(0, 0, 0), normalize(float3(-1, 0, -1)));
			}
		}

		if(actor && actor->isDead())
			actor = nullptr;

		if(!m_is_exiting) {
			for(auto event : device.inputEvents())
				onInput(event);
			m_is_exiting = m_hud->exitRequested();
		}

		m_console->update(time_diff);
		m_hud->update(time_diff);
		m_target_info->update(time_diff);

		if(GameMode *game_mode = m_world->gameMode()) {
			UserMessage message = game_mode->userMessage(UserMessageType::main);

			bool not_empty = !message.text.empty();
			hud::animateValue(m_main_message.anim_time, time_diff * 10.0f, not_empty);
			if(not_empty || m_main_message.anim_time < 0.01f)
				m_main_message.message = message;
		}

		{
			Actor *target_actor = m_world->refEntity<Actor>(m_isect);
			float2 info_size = m_target_info->rect().size();
			float2 pos = (float2)m_last_mouse_pos - float2(info_size.x * 0.5f, info_size.y + 30.0f);
			pos.x = clamp(pos.x, 0.0f, (float)resolution.x - info_size.x);
			if(pos.y < 0.0f)
				pos.y = m_last_mouse_pos.y + 40.0f;
			m_target_info->setPos(pos);

			float hit_chance = 0.0f;
			if(actor) {
				const Weapon &weapon = actor->inventory().weapon();
				if(!m_isect.empty() && weapon.hasRangedAttack()) {
					FBox bbox = m_viewer.refBBox(m_isect);
					hit_chance = actor->estimateHitChance(actor->inventory().weapon(), bbox);
				}
			}

			const PlayableCharacter *pc =
				target_actor ? m_game_mode->pc(m_game_mode->findPC(target_actor->ref())) : nullptr;
			const GameModeClient *gm_client = dynamic_cast<const GameModeClient *>(m_game_mode);
			if(gm_client && target_actor) {
				auto stats = gm_client->stats();
				for(const auto &stat : stats)
					if(stat.client_id == target_actor->clientId()) {
						m_target_info->setStats(stat);
						break;
					}
			}

			if(pc) {
				m_target_info->setCharacter(make_shared<Character>(pc->character()));
				m_target_info->setHealth(target_actor->hitPoints() /
										 target_actor->proto().actor->hit_points);
				m_target_info->setHitChance(hit_chance);
				m_target_info->setName(m_game_mode->client(target_actor->clientId())->nick_name);
			}

			m_target_info->setVisible(pc != nullptr);
		}

		while(true) {
			string command = m_console->getCommand();
			if(command.empty())
				break;

			using namespace xml_conversions;

			auto strings = fromString<vector<string>>(command);
			if(strings.size() != 2) {
				printf("Invalid command: %s\n", command.c_str());
				continue;
			}
			const char *param = strings[1].c_str();

			if(strings[0] == "debug")
				m_show_debug_info = fromString<bool>(param);
			else if(strings[0] == "navi_debug")
				m_debug_navi = fromString<bool>(param);
			else if(strings[0] == "ai_debug")
				m_debug_ai = fromString<bool>(param);
			else if(strings[0] == "time_mul")
				m_time_multiplier = clamp(fromString<float>(param), 0.0f, 10.0f);
			else if(strings[0] == "see_all")
				m_viewer.setSeeAll(fromString<bool>(param));
			else
				printf("Invalid command: %s\n", strings[0].c_str());
		}
	}

	void Controller::updateView(double time_diff) { m_viewer.update(time_diff); }

	void Controller::draw() const {
		GfxDevice::clearColor(Color(0, 0, 0));

		auto resolution = GfxDevice::instance().windowSize();
		IRect viewport(resolution);
		SceneRenderer scene_renderer(viewport, m_view_pos);
		m_viewer.addToRender(scene_renderer);

		if(!m_isect.empty())
			scene_renderer.addBox(m_world->refBBox(m_isect), Color::yellow);

		if(m_debug_navi) {
			const NaviMap *navi_map = m_world->naviMap(m_actor_ref);
			if(navi_map)
				navi_map->visualize(scene_renderer, false);
		}
		m_last_path.visualize(3, scene_renderer);
		scene_renderer.render();

		Renderer2D ui_renderer(viewport);

		if(m_show_debug_info)
			drawDebugInfo(ui_renderer);

		m_hud->draw(ui_renderer);
		m_target_info->draw(ui_renderer);
		m_console->draw(ui_renderer);

		if(!m_main_message.empty()) {
			PFont font = res::getFont("transformers_48");
			FRect rect(float2(resolution.x, 30.0f));
			rect += float2(0.0f, m_console->rect().height());
			Color text_color = mulAlpha(Color::white, m_main_message.anim_time);
			Color shadow_color = mulAlpha(Color::black, m_main_message.anim_time);

			font->draw(ui_renderer, rect, {text_color, shadow_color, HAlign::center},
					   m_main_message.text());
		}
		ui_renderer.render();
	}

	void Controller::drawDebugInfo(Renderer2D &out) const {
		PFont font = res::getFont("liberation_16");

		if(m_debug_ai) {
			for(int n = 0; n < m_world->entityCount(); n++) {
				const ThinkingEntity *entity = m_world->refEntity<ThinkingEntity>(n);
				const Brain *ai = entity ? entity->AI() : nullptr;
				if(ai) {
					string status = ai->status();
					FRect screen_rect = worldToScreen(entity->boundingBox());
					FRect text_rect = FRect(float2(200, 50)) +
									  float2(screen_rect.center().x, screen_rect.min.y) -
									  float2(m_view_pos);
					text_rect -= float2(text_rect.width() * 0.5f, 0.0f);
					font->draw(out, text_rect, {Color::white, Color::black, HAlign::center},
							   status);
				}
			}
		}

		TextFormatter fmt(4096);

		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		Actor *target_actor = m_world->refEntity<Actor>(m_isect);
		float3 isect_pos = m_screen_ray.at(m_isect.distance());

		const char *navi_status = "";
		if(m_debug_navi) {
			const NaviMap *navi_map = m_world->naviMap(m_actor_ref);
			if(navi_map) {
				int src = navi_map->findQuad((int3)actor->pos(), actor->ref().index());
				int dst = navi_map->findQuad((int3)isect_pos);
				navi_status = navi_map->isReachable(src, dst) ? "reachable" : "unreachable";
			}
		}
		fmt("View:(%d %d)\nRay:(%.2f %.2f %.2f) %s\n", m_view_pos.x, m_view_pos.y, isect_pos.x,
			isect_pos.y, isect_pos.z, navi_status);

		if(actor) {
			float3 actor_pos = actor->pos();
			fmt("Actor pos:(%.0f %.0f %.0f)\nHP: %d ", actor_pos.x, actor_pos.y, actor_pos.z,
				actor->hitPoints());
			if(target_actor)
				fmt("Target HP: %d", target_actor->hitPoints());

			const Weapon &weapon = actor->inventory().weapon();
			if(!m_isect.empty() && weapon.hasRangedAttack()) {
				FBox bbox = m_viewer.refBBox(m_isect);
				float hit_chance = actor->estimateHitChance(actor->inventory().weapon(), bbox);
				fmt("\nHit chance: %.0f%%", hit_chance * 100.0f);
			}
			fmt("\n\n");
		}

		fmt("%s", s_profiler_stats.c_str());

		int2 extents = font->evalExtents(fmt.text()).size();
		extents.y = (extents.y + 19) / 20 * 20;
		int2 pos = out.viewport().max - extents - int2(4, 4);
		out.addFilledRect(FRect((float2)pos, (float2)out.viewport().size()), Color(0, 0, 0, 80));
		font->draw(out, (float2)(pos + int2(2, 2)), {Color::white, Color::black}, fmt);
	}
}
