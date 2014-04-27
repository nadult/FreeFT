/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/io.h"
#include "sys/profiler.h"

#include "game/actor.h"
#include "game/item.h"
#include "game/container.h"
#include "game/visibility.h"

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"
#include "audio/device.h"

using namespace gfx;
using namespace game;

namespace io {

	IO::IO(const int2 &resolution, PWorld world, WorldViewer &viewer, EntityRef actor_ref, bool show_stats)
		:m_console(resolution), m_world(world), m_viewer(viewer), m_actor_ref(actor_ref), m_resolution(resolution),
		 m_view_pos(0, 0), m_inventory_sel(-1), m_container_sel(-1), m_show_stats(show_stats)  {
			DASSERT(world);
			const Actor *actor = m_world->refEntity<Actor>(actor_ref);
			if(actor)
				m_view_pos = int2(worldToScreen(actor->pos())) - resolution / 2;
		
			m_last_time = m_stats_update_time = getTime();
			m_last_look_at = float3(0, 0, 0);
	
			if(audio::isInitialized()) {
				audio::setListenerPos(float3(0, 0, 0));
				audio::setListenerVelocity(float3(0, 0, 0));
				audio::setUnits(16.66666666);
			}
		}

	void IO::update() {
		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();
		
		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(actor && actor->isDead())
			actor = nullptr;
		if(actor)
			audio::setListenerPos(actor->pos());

		int2 mouse_pos = getMousePos();
		bool console_mode = m_console.isOpened();

		Ray ray = screenRay(mouse_pos + m_view_pos);
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
			float3 target = ray.at(m_full_isect.distance());
			float3 origin = actor->boundingBox().center();
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			m_shoot_isect = m_world->trace(Segment(shoot_ray, 0.0f), {Flags::all | Flags::colliding, m_actor_ref});
		}

		if(!console_mode && isKeyDown('T') && !m_isect.isEmpty() && actor) {
			float3 pos = ray.at(m_isect.distance());
			actor->setPos(int3(pos + float3(0.5f, 0.5f, 0.5f)));
			actor->fixPosition();
		}

		m_console.processInput();
			

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
		if(!console_mode && (isKeyDown(Key_kp_add) || isKeyDown(Key_kp_subtract))) {
			Stance::Type stance = (Stance::Type)(actor->stance() + (isKeyDown(Key_kp_add)? 1 : -1));
			if(Stance::isValid(stance))
				m_world->sendOrder(new ChangeStanceOrder(stance), m_actor_ref);
		}

		if(actor) {
			Container *container = m_world->refEntity<Container>(m_isect);
			if(container && !(container->isOpened() && areAdjacent(*actor, *container)))
				container = nullptr;
			m_container_ref = container? container->ref() : EntityRef();

			if(isKeyPressed(Key_lctrl)) {
				if(isKeyDown(Key_up))
					m_container_sel--;
				if(isKeyDown(Key_down))
					m_container_sel++;
			}
			else {
				if(isKeyDown(Key_up))
					m_inventory_sel--;
				if(isKeyDown(Key_down))
					m_inventory_sel++;
			}

			m_last_path = actor->currentPath();
			if(!m_isect.isEmpty() && m_last_path.isEmpty())
				m_world->findPath(m_last_path, (int3)actor->pos(), (int3)ray.at(m_isect.distance()), m_actor_ref);

			m_inventory_sel = clamp(m_inventory_sel, -3, actor->inventory().size() - 1);
			m_container_sel = clamp(m_container_sel, 0, container? container->inventory().size() - 1 : 0);

			if(isKeyDown('D') && m_inventory_sel >= 0)
				m_world->sendOrder(new DropItemOrder(m_inventory_sel), m_actor_ref);
			else if(isKeyDown('E') && m_inventory_sel >= 0)
				m_world->sendOrder(new EquipItemOrder(m_inventory_sel), m_actor_ref);
			else if(isKeyDown('E') && m_inventory_sel < 0) {
				ItemType::Type type = ItemType::Type(m_inventory_sel + 3);
				m_world->sendOrder(new UnequipItemOrder(type), m_actor_ref);
			}

			if(container) {
				if(isKeyDown(Key_right) && m_inventory_sel >= 0)
					m_world->sendOrder(new TransferItemOrder(container->ref(), transfer_to, m_inventory_sel, 1), m_actor_ref);
				if(isKeyDown(Key_left))
					m_world->sendOrder(new TransferItemOrder(container->ref(), transfer_from, m_container_sel, 1), m_actor_ref);
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

	void IO::draw() {
		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), m_resolution), m_view_pos);

		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		Ray ray = screenRay(getMousePos() + m_view_pos);

		m_viewer.addToRender(renderer);

		if(!m_isect.isEmpty())
			renderer.addBox(m_world->refBBox(m_isect), Color::yellow);

		if(!m_shoot_isect.isEmpty()) {
			FBox box = m_world->refBBox(m_shoot_isect);
			renderer.addBox(box, Color(255, 0, 0, 100));
		}

		Actor *target_actor = m_world->refEntity<Actor>(m_isect);

//		const NaviMap *navi_map = m_world->naviMap(3);
//		if(navi_map)
//			navi_map->visualize(renderer, false);
//		m_last_path.visualize(3, renderer);
		renderer.render();

		lookAt(m_view_pos);
			
		lookAt({0, 0});
		drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
		drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

		DTexture::bind0();
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
			fmt("\n");
			fmt("\n");
		}

		if(m_show_stats)
			fmt("%s", m_profiler_stats.c_str());
		
		int2 extents = font->evalExtents(fmt.text()).size();
		extents.y = (extents.y + 19) / 20 * 20;
		drawQuad(0, 0, extents.x + 4, extents.y + 4, Color(0, 0, 0, 80));
		font->drawShadowed(int2(2, 2), Color::white, Color::black,fmt.text());

		lookAt({0, 0});
		if(actor) {
			Container *container = m_world->refEntity<Container>(m_container_ref);
			string inv_info = actor? actor->inventory().printMenu(m_inventory_sel) : string();
			string cont_info = container? container->inventory().printMenu(m_container_sel) : string();
			if(container)
				cont_info = string("Container: ") + container->proto().id + "\n" + cont_info;
				
			IRect extents = font->evalExtents(inv_info.c_str());
			font->drawShadowed(int2(0, m_resolution.y - extents.height()),
							Color::white, Color::black, "%s", inv_info.c_str());

			extents = font->evalExtents(cont_info.c_str());
			font->drawShadowed(int2(m_resolution.x - extents.width(), m_resolution.y - extents.height()),
							Color::white, Color::black, "%s", cont_info.c_str());
		}

		m_console.draw();
	}

	void IO::drawVisibility(game::EntityRef ref) {
	}

}
