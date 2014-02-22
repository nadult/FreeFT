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
	game::loadPools();

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
	World world(World::Mode::single_player, map_name.c_str());

	int height = 128;

	Actor *actor = new Actor(ActorTypeId::male, float3(245, height, 335));
	int actor_id = world.addEntity(actor);

/*	Container *chest = world.addEntity(new Container("containers/Chest Wooden", float3(245, height, 340)));
	Container *toolbench = world.addEntity(new Container("containers/Toolbench S", float3(260, height, 350)));
	Container *fridge = world.addEntity(new Container("containers/Fridge S", float3(250, height, 340)));
//	actor = world.addEntity(new Actor(ActorTypeId::mutant, float3(260, height, 335)));
//	world.addEntity(new Container("containers/Ice Chest N", float3(240, height, 345)));

//	Door *door = world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(495, height, 342), DoorTypeId::rotating));
//	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(495, height, 382), DoorTypeId::rotating, float2(1, 0)));
//	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(495, height, 392), DoorTypeId::rotating, float2(-1, 0)));
//	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(485, height, 382), DoorTypeId::rotating, float2(0, 1)));
//	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(485, height, 392), DoorTypeId::rotating, float2(0, -1)));
//	world.addEntity(new Door("doors/BOS DOORS/BOS InteriorDoor2", float3(475, height, 392), DoorTypeId::sliding, float2(0, -1)));
	chest->setDir(float2(0, -1));
	fridge->setDir(float2(-1, 0));
//	door->setKey(ItemDesc::find("prison_key"));
	fridge->setKey(ItemDesc::find("prison_key"));

	chest->inventory().add(ItemDesc::find("laser_rifle"), 1);
	chest->inventory().add(ItemDesc::find("plasma_rifle"), 1);
	chest->inventory().add(ItemDesc::find("fusion_cell"), 100);
	chest->inventory().add(ItemDesc::find("prison_key"), 1);
	fridge->inventory().add(ItemDesc::find("power_armour"), 1);
	fridge->inventory().add(ItemDesc::find("m60"), 1); */

//	world.addEntity(new ItemEntity(ItemDesc::find("leather_armour"), float3(125, height, 60)));

	world.updateNaviMap(true);

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
			if(isect.entity() && entity_debug) {
				//isect.entity->interact(nullptr);
				InteractionMode mode = isect.entity()->entityType() == EntityId::item?
					interact_pickup : interact_normal;
				actor->setNextOrder(interactOrder(isect.entity(), mode));
			}
			else if(navi_debug) {
				//TODO: do this on floats, in actor and navi code too
				int3 wpos = (int3)(ray.at(isect.distance()));
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));

			}
			else if(isect.isTile()) {
				//TODO: pixel intersect always returns distance == 0
				int3 wpos = int3(ray.at(isect.distance()) + float3(0, 0.5f, 0));
				actor->setNextOrder(moveOrder(wpos, !isKeyPressed(Key_lshift)));
			}
		}
		if(isMouseKeyDown(1) && shooting_debug) {
			AttackMode::Type mode = isKeyPressed(Key_lshift)? AttackMode::burst : AttackMode::undefined;
			actor->setNextOrder(attackOrder(mode, (int3)target_pos));
		}
		if((navi_debug || (navi_show && !shooting_debug)) && isMouseKeyDown(1)) {
			int3 wpos = (int3)ray.at(isect.distance());
			path = world.findPath(last_pos, wpos);
			last_pos = wpos;
		}
		if(isKeyDown(Key_kp_add))
			actor->setNextOrder(changeStanceOrder(1));
		if(isKeyDown(Key_kp_subtract))
			actor->setNextOrder(changeStanceOrder(-1));

		if(isKeyDown('R') && navi_debug) {
			world.naviMap().removeColliders();
		}

		double time = getTime();
		if(!navi_debug)
			world.updateNaviMap(false);

		audio::tick();
		world.simulate((time - last_time) * config.time_multiplier);

		last_time = time;

		clear(Color(128, 64, 0));
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		world.updateVisibility(actor->boundingBox());

		world.addToRender(renderer);

		if((entity_debug && isect.isEntity()) || 1)
			renderer.addBox(isect.boundingBox(), Color::yellow);

		if(!full_isect.isEmpty() && shooting_debug) {
			float3 target = ray.at(full_isect.distance());
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world.trace(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = shoot_isect.boundingBox();
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

			Container *container = dynamic_cast<Container*>(isect.entity());
			if(container && !(container->isOpened() && areAdjacent(*actor, *container)))
				container = nullptr;

			inventory_sel = clamp(inventory_sel, -3, actor->inventory().size() - 1);
			container_sel = clamp(container_sel, 0, container? container->inventory().size() - 1 : 0);

			if(isKeyDown('D') && inventory_sel >= 0)
				actor->setNextOrder(dropItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel >= 0)
				actor->setNextOrder(equipItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel < 0) {
				ItemType::Type type = ItemType::Type(inventory_sel + 3);
				actor->setNextOrder(unequipItemOrder(type));
			}

			if(container) {
				if(isKeyDown(Key_right) && inventory_sel >= 0)
					actor->setNextOrder(transferItemOrder(container, transfer_to, inventory_sel, 1));
				if(isKeyDown(Key_left))
					actor->setNextOrder(transferItemOrder(container, transfer_from, container_sel, 1));
			}

			string inv_info = actor->inventory().printMenu(inventory_sel);
			string cont_info = container? container->inventory().printMenu(container_sel) : string();
				
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

