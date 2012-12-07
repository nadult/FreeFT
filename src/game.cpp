#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"

#include "tile_map.h"
#include "navigation_map.h"
#include "navigation_bitmap.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/container.h"
#include "game/door.h"

using namespace gfx;
using namespace game;



int safe_main(int argc, char **argv)
{
#if defined(RES_X) && defined(RES_Y)
	int2 res(RES_X, RES_Y);
#else
	int2 res(1400, 768);
#endif

	createWindow(res, false);
	setWindowTitle("FTremake ver 0.02");
	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(0, 0);

	PFont font = Font::mgr["arial_32"];

	World world("data/tile_map.xml");

	Actor *actor = world.addEntity(new Actor("characters/LeatherMale", int3(100, 1, 70)));
	actor->setNextOrder(changeWeaponOrder(WeaponClassId::rifle));

	Container *chest = world.addEntity(new Container("containers/Chest Wooden", int3(134, 1, 37)));
	Container *toolbench = world.addEntity(new Container("containers/Toolbench S", int3(120, 1, 37)));
	world.addEntity(new Container("containers/Fridge S", int3(134, 1, 25)));
	world.addEntity(new Container("containers/Ice Chest N", int3(120, 1, 25)));

	Door *door = world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", int3(95, 1, 42)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", int3(95, 1, 82), float2(1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", int3(95, 1, 92), float2(-1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", int3(85, 1, 82), float2(0, 1)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", int3(85, 1, 92), float2(0, -1)));
	chest->setDir(float2(0, -1));

	world.updateNavigationMap(true);

	printf("Actor size: %d %d %d\n",
			actor->boundingBox().width(),
			actor->boundingBox().height(),
			actor->boundingBox().depth());


	bool navi_show = 0;
	bool navi_debug = 0;
	bool shooting_debug = 0;
	bool entity_debug = 1;
	
	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0), target_pos(0, 0, 0);

	const TileMap &tile_map = world.tileMap();

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Ray ray = screenRay(getMousePos() + view_pos);
		auto isect = tile_map.intersect(ray, -1.0f/0.0f, 1.0f/0.0f);
		auto ent_isect = world.intersectEntities(ray, -1.0f/0.0f, 1.0f/0.0f);

		if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
			if(ent_isect.entity && entity_debug) {
				if(ent_isect.entity != actor)
					ent_isect.entity->interact(nullptr);
					//actor->setNextOrder(interactOrder(ent_isect.entity));
			}
			else if(navi_debug) {
				int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
//				int quad_id = world.naviMap().findQuad(wpos.xz());
//				if(quad_id != -1)
//					world.naviMap()[quad_id].is_disabled ^= 1;
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));

			}
			else {
				int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
				actor->setNextOrder(moveOrder(wpos, isKeyPressed(Key_lshift)));
			}
		}
		if(isMouseKeyDown(1) && shooting_debug) {
			actor->setNextOrder(attackOrder(0, target_pos));
		}
		if(navi_debug && isMouseKeyDown(1)) {
			int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
			path = world.findPath(last_pos.xz(), wpos.xz());
			last_pos = wpos;
		}
		if(isKeyDown(Key_kp_add))
			actor->setNextOrder(changeStanceOrder(1));
		if(isKeyDown(Key_kp_subtract))
			actor->setNextOrder(changeStanceOrder(-1));

		if(isKeyDown('R') && navi_debug) {
			world.naviMap().removeColliders();
		}
		if(isKeyDown('W') && shooting_debug)
			actor->setNextOrder(
				changeWeaponOrder((WeaponClassId::Type)((actor->weaponId() + 1) % WeaponClassId::count)));

		double time = getTime();
		if(!navi_debug)
			world.updateNavigationMap(false);

		world.simulate((time - last_time));
		last_time = time;

		clear({128, 64, 0});
		SceneRenderer renderer(IRect(0, 0, res.x, res.y), view_pos);

		world.addToRender(renderer);

		if(entity_debug && ent_isect.entity) {
			IBox box = ent_isect.entity->boundingBox();
			renderer.addBox(box);
		}	

		if(isect.node_id != -1 && shooting_debug) {
			IBox box = tile_map(isect.node_id)(isect.instance_id).boundingBox();
			box += tile_map.nodePos(isect.node_id);
			renderer.addBox(box);

			float3 target = ray.at(isect.t);
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			auto shoot_isect = tile_map.intersect(shoot_ray, -10.0f, 1.0f/0.0f);

			if(shoot_isect.node_id != -1) {
				IBox box = tile_map(shoot_isect.node_id)(shoot_isect.instance_id).boundingBox();
				box += tile_map.nodePos(shoot_isect.node_id);
				renderer.addBox(box, Color(255, 0, 0));
				target_pos = (int3)(shoot_ray.at(shoot_isect.t));
			}
		}

		if(navi_debug || navi_show) {
			world.naviMap().visualize(renderer, true);
			world.naviMap().visualizePath(path, 1, renderer);
		}

		renderer.render();
		lookAt(view_pos);

		{
			lookAt({0, 0});
			
			drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
			drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

			gfx::PFont font = gfx::Font::mgr["arial_24"];

			int3 pos = (int3)ray.at(isect.t);

			font->drawShadowed(int2(0, 0), Color::white, Color::black, "(%d %d %d)", pos.x, pos.y, pos.z);
			font->drawShadowed(int2(0, 20), Color::white, Color::black, "%d %d %f",
					isect.node_id, isect.instance_id, isect.t);

//			double time = GetTime();
//			double frameTime = time - lastFrameTime;
//			lastFrameTime = time;
			
//			string profData = Profiler::getStats();
//			Profiler::nextFrame();
//			printf("%s\n", profData.c_str());
		}

		swapBuffers();
	}

	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		destroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

