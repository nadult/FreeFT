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
		}

	void IO::update() {
		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			m_view_pos -= getMouseMove();
		
		Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		int2 mouse_pos = getMousePos();

		Ray ray = screenRay(mouse_pos + m_view_pos);
		m_isect = m_viewer.pixelIntersect(mouse_pos + m_view_pos, collider_tile_walkable|collider_entities);
		if(m_isect.isEmpty() || m_isect.isTile())
			m_isect = m_viewer.trace(ray, actor, collider_tile_walkable|collider_entities);
		
		//TODO: pixel intersect may find an intersection, but the ray doesn't necessarily
		// has to intersect bounding box of the object
		m_full_isect = m_viewer.pixelIntersect(mouse_pos + m_view_pos, collider_all);
		if(m_full_isect.isEmpty())
			m_full_isect = m_viewer.trace(ray, actor, collider_all);

		if(!m_full_isect.isEmpty() && actor) {
			float3 target = ray.at(m_full_isect.distance());
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			m_shoot_isect = m_world->trace(Segment(shoot_ray, 0.0f), actor);

			if(!m_shoot_isect.isEmpty())
				m_target_pos = shoot_ray.at(m_shoot_isect.distance());
		}

		if(isKeyDown('T') && !m_isect.isEmpty() && actor)
			actor->setPos(ray.at(m_isect.distance()));

		m_console.processInput();

		if(!m_full_isect.isEmpty()) {
			//TODO: send it only, when no other order is in progress (or has been sent and wasn't finished)
			if(m_full_isect.distance() < constant::inf && m_full_isect.distance() > -constant::inf) {
				float3 look_at = ray.at(m_full_isect.distance());
				if(look_at != m_last_look_at) {
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
				m_world->sendOrder(new MoveOrder(wpos, !isKeyPressed(Key_lshift)), m_actor_ref);
			}
		}
		if(isMouseKeyDown(1)) {
			AttackMode::Type mode = AttackMode::undefined;
			if(isKeyPressed(Key_lshift)) {
				const Weapon &weapon = actor->inventory().weapon();
				if(weapon.proto().attack_modes & AttackModeFlags::burst)
					mode = AttackMode::burst;
				else if(weapon.proto().attack_modes & AttackModeFlags::kick)
					mode = AttackMode::kick;
			}
			m_world->sendOrder(new AttackOrder(mode, (int3)m_target_pos), m_actor_ref);
		}
		if(isKeyDown(Key_kp_add) || isKeyDown(Key_kp_subtract)) {
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

			m_last_path = actor->getPath();
			if(!m_isect.isEmpty() && m_last_path.empty())
				m_last_path = m_world->findPath((int3)actor->pos(), (int3)ray.at(m_isect.distance()), m_actor_ref);

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

		m_world->naviMap().visualize(renderer, false);
		if(!m_last_path.empty())
			m_world->naviMap().visualizePath(m_last_path, 3, renderer);
		renderer.render();

		lookAt(m_view_pos);
			
		lookAt({0, 0});
		drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
		drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

		DTexture::bind0();
		lookAt({0, -m_console.size().y});
		drawQuad(0, 0, 280, m_show_stats? 250 : 50, Color(0, 0, 0, 80));
		
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
		float3 isect_pos = ray.at(m_isect.distance());

		font->drawShadowed(int2(0, 0), Color::white, Color::black,
				"View:(%d %d)\nRay:(%.2f %.2f %.2f)",
				m_view_pos.x, m_view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z);
		if(actor) {
			float3 actor_pos = actor->pos();
			font->drawShadowed(int2(0, font->lineHeight() * 2), Color::white, Color::black,
					"Actor:(%.2f %.2f %.2f)", actor_pos.x, actor_pos.y, actor_pos.z);
		}

		if(m_show_stats)
			font->drawShadowed(int2(0, 60), Color::white, Color::black, "%s", m_profiler_stats.c_str());

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
