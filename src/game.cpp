/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/scene_renderer.h"

#include "navi_map.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/container.h"
#include "game/door.h"
#include "game/item.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "audio/device.h"

using namespace gfx;
using namespace game;


int safe_main(int argc, char **argv)
{
	audio::initSoundMap();
	Config config = loadConfig("game");
	game::loadData(true);

	audio::initDevice();

	audio::setListenerPos(float3(0, 0, 0));
	audio::setListenerVelocity(float3(0, 0, 0));
	audio::setUnits(16.66666666);

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::game; built " __DATE__ " " __TIME__);

	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(-1000, 500);

	PFont font = Font::mgr["liberation_32"];

	string map_name = "data/maps/mission05.mod";
	if(argc > 1)
		map_name = string("data/maps/") + argv[1];
	World world(map_name.c_str(), World::Mode::single_player);

	int height = 128;

	EntityRef actor_ref; {
		Actor *actor = new Actor(getProto("male", ProtoId::actor));
		actor->setPos(float3(245, height, 335));
		world.addEntity(PEntity(actor));
		actor_ref = actor->ref();
	}

	bool navi_show = 0;
	bool navi_debug = 0;
	bool shooting_debug = 1;
	bool entity_debug = 1;
	bool item_debug = 1;
	
	double last_time = getTime();
	vector<int3> path;
	int3 last_pos(0, 0, 0);
	float3 target_pos(0, 0, 0);

	int inventory_sel = -1, container_sel = -1;
	string prof_stats;
	double stat_update_time = getTime();

	while(pollEvents()) {
		double loop_start = profiler::getTime();
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Actor *actor = world.refEntity<Actor>(actor_ref);

		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world.pixelIntersect(getMousePos() + view_pos,
				collider_tile_floors|collider_tile_roofs|collider_tile_stairs|collider_entities|visibility_flag);
		if(isect.isEmpty() || isect.isTile())
			isect = world.trace(ray, actor,
				collider_tile_floors|collider_tile_roofs|collider_tile_stairs|collider_entities|visibility_flag);
		
		Intersection full_isect = world.pixelIntersect(getMousePos() + view_pos, collider_all|visibility_flag);
		if(full_isect.isEmpty())
			full_isect = world.trace(ray, actor, collider_all|visibility_flag);

		
		if(isKeyDown('T') && !isect.isEmpty())
			actor->setPos(ray.at(isect.distance()));

		if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
			Entity *entity = world.refEntity(isect);

			if(entity && entity_debug) {
				world.sendOrder(new InteractOrder(entity->ref()), actor_ref);
			}
			else if(navi_debug) {
				//TODO: do this on floats, in actor and navi code too
				int3 wpos = (int3)(ray.at(isect.distance()));
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));
			}
			else if(isect.isTile()) {
				//TODO: pixel intersect always returns distance == 0
				int3 wpos = int3(ray.at(isect.distance()) + float3(0, 0.5f, 0));
				world.sendOrder(new MoveOrder(wpos, !isKeyPressed(Key_lshift)), actor_ref);
			}
		}
		if(isMouseKeyDown(1) && shooting_debug) {
			AttackMode::Type mode = isKeyPressed(Key_lshift)? AttackMode::burst : AttackMode::undefined;
			world.sendOrder(new AttackOrder(mode, (int3)target_pos), actor_ref);
		}
	/*	if((navi_debug || (navi_show && !shooting_debug)) && isMouseKeyDown(1)) {
			int3 wpos = (int3)ray.at(isect.distance());
			path = world.findPath(last_pos, wpos);
			last_pos = wpos;
		}*/
		if(isKeyDown(Key_kp_add) || isKeyDown(Key_kp_subtract)) {
			Stance::Type stance = (Stance::Type)(actor->stance() + (isKeyDown(Key_kp_add)? 1 : -1));
			if(Stance::isValid(stance))
				world.sendOrder(new ChangeStanceOrder(stance), actor_ref);
		}

		if(isKeyDown('R') && navi_debug) {
			world.naviMap().removeColliders();
		}

		double time = getTime();
		if(!navi_debug)
			world.updateNaviMap(false);

		audio::tick();
		world.simulate((time - last_time) * config.time_multiplier);
		
		actor = dynamic_cast<Actor*>(world.refEntity(actor_ref));
		ASSERT(actor);

		last_time = time;

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		world.updateVisibility(actor->boundingBox());

		world.addToRender(renderer);

		if((entity_debug && isect.isEntity()) || 1)
			renderer.addBox(world.refBBox(isect), Color::yellow);

		if(!full_isect.isEmpty() && shooting_debug) {
			float3 target = ray.at(full_isect.distance());
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world.trace(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = world.refBBox(shoot_isect);
				renderer.addBox(box, Color(255, 0, 0, 100));
				target_pos = shoot_ray.at(shoot_isect.distance());
			}
		}

		if(navi_debug || navi_show) {
			world.naviMap().visualize(renderer, true);
			world.naviMap().visualizePath(path, 3, renderer);
		}

		renderer.render();
		lookAt(view_pos);
			
		lookAt({0, 0});
		drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
		drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

		DTexture::bind0();
		drawQuad(0, 0, 250, config.profiler_enabled? 300 : 50, Color(0, 0, 0, 80));
		
		gfx::PFont font = gfx::Font::mgr["liberation_16"];
		float3 isect_pos = ray.at(isect.distance());
		float3 actor_pos = actor->pos();
		font->drawShadowed(int2(0, 0), Color::white, Color::black,
				"View:(%d %d)\nRay:(%.2f %.2f %.2f)\nActor:(%.2f %.2f %.2f)",
				view_pos.x, view_pos.y, isect_pos.x, isect_pos.y, isect_pos.z, actor_pos.x, actor_pos.y, actor_pos.z);
		if(config.profiler_enabled)
			font->drawShadowed(int2(0, 60), Color::white, Color::black, "%s", prof_stats.c_str());

		if(item_debug) {
			if(isKeyPressed(Key_lctrl)) {
				if(isKeyDown(Key_up))
					container_sel--;
				if(isKeyDown(Key_down))
					container_sel++;
			}
			else {
				if(isKeyDown(Key_up))
					inventory_sel--;
				if(isKeyDown(Key_down))
					inventory_sel++;
			}

			Container *container = world.refEntity<Container>(isect);
			if(container && !(container->isOpened() && areAdjacent(*actor, *container)))
				container = nullptr;

			inventory_sel = clamp(inventory_sel, -3, actor->inventory().size() - 1);
			container_sel = clamp(container_sel, 0, container? container->inventory().size() - 1 : 0);

			if(isKeyDown('D') && inventory_sel >= 0)
				world.sendOrder(new DropItemOrder(inventory_sel), actor_ref);
			else if(isKeyDown('E') && inventory_sel >= 0)
				world.sendOrder(new EquipItemOrder(inventory_sel), actor_ref);
			else if(isKeyDown('E') && inventory_sel < 0) {
				ItemType::Type type = ItemType::Type(inventory_sel + 3);
				world.sendOrder(new UnequipItemOrder(type), actor_ref);
			}

			if(container) {
				if(isKeyDown(Key_right) && inventory_sel >= 0)
					world.sendOrder(new TransferItemOrder(container->ref(), transfer_to, inventory_sel, 1), actor_ref);
				if(isKeyDown(Key_left))
					world.sendOrder(new TransferItemOrder(container->ref(), transfer_from, container_sel, 1), actor_ref);
			}

			string inv_info = actor->inventory().printMenu(inventory_sel);
			string cont_info = container? container->inventory().printMenu(container_sel) : string();
			if(container)
				cont_info = string("Container: ") + container->proto().id + "\n" + cont_info;
				
			IRect extents = font->evalExtents(inv_info.c_str());
			font->drawShadowed(int2(0, config.resolution.y - extents.height()),
							Color::white, Color::black, "%s", inv_info.c_str());

			extents = font->evalExtents(cont_info.c_str());
			font->drawShadowed(int2(config.resolution.x - extents.width(), config.resolution.y - extents.height()),
							Color::white, Color::black, "%s", cont_info.c_str());
		}

		swapBuffers();
		TextureCache::main_cache.nextFrame();

		profiler::updateTimer("main_loop", profiler::getTime() - loop_start);
		if(getTime() - stat_update_time > 0.25) {
			prof_stats = profiler::getStats();
			stat_update_time = getTime();
		}
		profiler::nextFrame();
	}

/*	PTexture atlas = TextureCache::main_cache.atlas();
	Texture tex;
	atlas->download(tex);
	Saver("atlas.tga") & tex; */

	audio::freeDevice();
	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		audio::freeDevice();
		destroyWindow();

		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

