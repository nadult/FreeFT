/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/controller.h"
#include "sys/profiler.h"

#include "game/actor.h"
#include "game/item.h"
#include "game/container.h"
#include "game/visibility.h"
#include "game/game_mode.h"

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"
#include "audio/device.h"

#include "gfx/texture.h"
#include "hud/hud.h"

using namespace gfx;
using namespace game;

namespace io {

	Controller::Controller(const int2 &resolution, PWorld world, bool show_stats)
	  :m_console(resolution), m_world(world), m_viewer(world), m_resolution(resolution), m_view_pos(0, 0), m_show_stats(show_stats) {
		DASSERT(world);
		m_hud = new hud::Hud(world);

		m_game_mode = m_world->gameMode();
		ASSERT(m_game_mode);
		updateActor();

		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(actor)
			m_view_pos = int2(worldToScreen(actor->pos())) - resolution / 2;
		
		m_last_time = m_stats_update_time = getTime();
		m_last_look_at = float3(0, 0, 0);
	
	}

	Controller::~Controller() {
	}
		
	//TODO: better name
	void Controller::updateActor() {
		const auto &pcs = m_game_mode->playableCharacters();
		const PlayableCharacter *pc = pcs.empty()? nullptr : &pcs[0];

		m_actor_ref = pc? pc->entityRef() : EntityRef();
		m_hud->setActor(m_actor_ref);
		m_viewer.setSpectator(m_actor_ref);
		m_hud->setCharacter(pc? new Character(pc->character()) : PCharacter());
	}

	void Controller::update(double time_diff) {
		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();
		
		updateActor();

		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(actor && actor->isDead())
			actor = nullptr;
		if(actor)
			audio::setListener(actor->pos(), actor->estimateMove(1.0f), normalized(float3(-1, 0, -1)));

		int2 mouse_pos = getMousePos();
		bool console_mode = m_console.isOpened();
		Ray ray = screenRay(mouse_pos + m_view_pos);

		m_console.update(time_diff);
		m_hud->update(!console_mode, time_diff);

		if(isKeyDown('H'))
			m_hud->setVisible(m_hud->isVisible() ^ 1);

		if(m_hud->isMouseOver()) {
			m_isect = m_full_isect = Intersection();
		}
		else {
			Flags::Type flags = Flags::walkable_tile | (Flags::entity & ~(Flags::projectile | Flags::impact | Flags::trigger));
			m_isect = m_viewer.pixelIntersect(mouse_pos + m_view_pos, {flags, m_actor_ref});
			if(m_isect.isEmpty() || m_isect.isTile())
				m_isect = m_viewer.trace(ray, {flags, m_actor_ref});
	
			//TODO: pixel intersect may find an intersection, but the ray doesn't necessarily
			// has to intersect bounding box of the object
			m_full_isect = m_viewer.pixelIntersect(mouse_pos + m_view_pos, m_actor_ref);
			if(m_full_isect.isEmpty())
				m_full_isect = m_viewer.trace(ray, m_actor_ref);

			if(!m_full_isect.isEmpty() && actor) {
				//TODO: send it only, when no other order is in progress (or has been sent and wasn't finished)
				if(m_full_isect.distance() < constant::inf && m_full_isect.distance() > -constant::inf) {
					float3 look_at = ray.at(m_full_isect.distance());
					if(look_at != m_last_look_at && distance(look_at.xz(), actor->boundingBox().center().xz()) > 1.0f) {
						m_world->sendOrder(new LookAtOrder(look_at), m_actor_ref);
						m_last_look_at = look_at;
					}
				}
			}

			if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
				Entity *entity = m_world->refEntity(m_isect);

				if(entity) {
					m_world->sendOrder(new InteractOrder(entity->ref()), m_actor_ref);
				}
				else if(m_isect.isTile()) {
					//TODO: pixel intersect always returns distance == 0
					int3 wpos = int3(ray.at(m_isect.distance()) + float3(0, 0.5f, 0));
					
					bool run = actor && !isKeyPressed(Key_lshift) && distance(float3(wpos), actor->pos()) > 10.0f;
					m_world->sendOrder(new MoveOrder(wpos, run), m_actor_ref);
				}
			}
			if(isMouseKeyDown(1) && actor) {
				AttackMode::Type mode = AttackMode::undefined;
				if(isKeyPressed(Key_lshift)) {
					const Weapon &weapon = actor->inventory().weapon();
					if(weapon.proto().attack_modes & AttackModeFlags::burst)
						mode = AttackMode::burst;
					else if(weapon.canKick())
						mode = AttackMode::kick;
				}

				if(!m_isect.isEmpty()) {
					Entity *entity = m_world->refEntity(m_isect);
					if(entity) {
						m_world->sendOrder(new AttackOrder(mode, entity->ref()), m_actor_ref);
					}
					else
						m_world->sendOrder(new AttackOrder(mode, ray.at(m_isect.distance())), m_actor_ref);
				}
			}

			if(!console_mode && isKeyDown('T') && !m_isect.isEmpty() && actor) {
				float3 pos = ray.at(m_isect.distance());
				actor->setPos(int3(pos + float3(0.5f, 0.5f, 0.5f)));
				actor->fixPosition();
			}

		}

		if(m_last_time - m_stats_update_time > 0.25) {
			m_profiler_stats = profiler::getStats();
			m_stats_update_time = m_last_time;
		}
		profiler::nextFrame();
		m_last_time = profiler::getTime();

		while(true) {
			string command = m_console.getCommand();
			if(command.empty())
				break;

			printf("Invalid command: %s\n", command.c_str());
		}
	}

	void Controller::updateView(double time_diff) {
		m_viewer.update(time_diff);
	}

	void Controller::draw() {
		clear(Color(0, 0, 0));
		SceneRenderer renderer(IRect(int2(0, 0), m_resolution), m_view_pos);

		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		Ray ray = screenRay(getMousePos() + m_view_pos);

		m_viewer.addToRender(renderer);

		if(!m_isect.isEmpty())
			renderer.addBox(m_world->refBBox(m_isect), Color::yellow);

		Actor *target_actor = m_world->refEntity<Actor>(m_isect);

		/*const NaviMap *navi_map = m_world->naviMap(3);
		if(navi_map)
			navi_map->visualize(renderer, false);
		m_last_path.visualize(3, renderer);*/
		renderer.render();

		lookAt(m_view_pos);

		/*{ // Drawing cursor
			lookAt({0, 0});
			DTexture::unbind();
			drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
			drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));
		}*/

		lookAt({0, -m_console.size().y});
		
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
		float3 isect_pos = ray.at(m_isect.distance());

		TextFormatter fmt(4096);

		fmt("View:(%d %d) Ray:(%.2f %.2f %.2f)\n", m_view_pos.x, m_view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z);

		if(actor) {
			float3 actor_pos = actor->pos();
			fmt("Actor pos:(%.0f %.0f %.0f)\nHP: %d ", actor_pos.x, actor_pos.y, actor_pos.z, actor->hitPoints());
			if(target_actor)
				fmt("Target HP: %d", target_actor->hitPoints());

			const Weapon &weapon = actor->inventory().weapon();
			if(!m_isect.isEmpty() && weapon.hasRangedAttack()) {
				FBox bbox = m_viewer.refBBox(m_isect);
				float hit_chance = actor->estimateHitChance(actor->inventory().weapon(), bbox);
				fmt("\nHit chance: %.0f%%", hit_chance * 100.0f);
			}
			fmt("\n\n");
		}

		if(m_show_stats)
			fmt("%s", m_profiler_stats.c_str());
		
		int2 extents = font->evalExtents(fmt.text()).size();
		extents.y = (extents.y + 19) / 20 * 20;
		int2 res = getWindowSize();
		int2 pos(res.x - extents.x - 4, res.y - extents.y - 4);
		drawQuad(pos.x, pos.y, res.x, res.y, Color(0, 0, 0, 80));
		font->draw(pos + int2(2, 2), {Color::white, Color::black}, fmt);

		lookAt({0, 0});
		m_console.draw();
		m_hud->draw();
	}

	void Controller::drawVisibility(game::EntityRef ref) {
	}

}
