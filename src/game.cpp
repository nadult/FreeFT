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
#include "game/item.h"
#include "sys/config.h"

using namespace gfx;
using namespace game;


int safe_main(int argc, char **argv)
{
	Config config = loadConfig("game");
	ItemDesc::loadItems();

	createWindow(config.resolution, config.fullscreen);

	setWindowTitle("FTremake ver 0.02");
	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(0, 0);

	PFont font = Font::mgr["arial_32"];

	World world("data/tile_map.xml");

	Actor *actor = world.addEntity(new Actor("characters/LeatherMale", float3(100, 1, 70)));
	actor->setNextOrder(changeWeaponOrder(WeaponClassId::rifle));

	Container *chest = world.addEntity(new Container("containers/Chest Wooden", float3(134, 1, 37)));
	Container *toolbench = world.addEntity(new Container("containers/Toolbench S", float3(120, 1, 37)));
	world.addEntity(new Container("containers/Fridge S", float3(134, 1, 25)));
	world.addEntity(new Container("containers/Ice Chest N", float3(120, 1, 25)));

	Door *door = world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 42), Door::type_rotating));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 82), Door::type_rotating, float2(1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 92), Door::type_rotating, float2(-1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(85, 1, 82), Door::type_rotating, float2(0, 1)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(85, 1, 92), Door::type_rotating, float2(0, -1)));
	world.addEntity(new Door("doors/BOS DOORS/BOS InteriorDoor2", float3(75, 1, 92), Door::type_sliding, float2(0, -1)));
	chest->setDir(float2(0, -1));

	const ItemDesc *plasma_rifle = ItemDesc::find("plasma_rifle");
	const ItemDesc *fusion_cell = ItemDesc::find("fusion_cell");
	const ItemDesc *leather_armour = ItemDesc::find("leather_armour");

	DASSERT(plasma_rifle && fusion_cell && leather_armour);

	world.addEntity(new Item(*plasma_rifle, float3(100, 1, 100)));

	world.updateNavigationMap(true);

	printf("Actor size: %.0f %.0f %.0f\n",
			actor->boundingBox().width(),
			actor->boundingBox().height(),
			actor->boundingBox().depth());


	bool navi_show = 0;
	bool navi_debug = 0;
	bool shooting_debug = 1;
	bool entity_debug = 1;
	
	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0);
	float3 target_pos(0, 0, 0);

	const TileMap &tile_map = world.tileMap();

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world.intersect(ray, actor);

		if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
			if(isect.entity() && entity_debug) {
				//isect.entity->interact(nullptr);
				actor->setNextOrder(interactOrder(isect.entity()));
			}
			else if(navi_debug) {
				int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));

			}
			else {
				int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
				actor->setNextOrder(moveOrder(wpos, isKeyPressed(Key_lshift)));
			}
		}
		if(isMouseKeyDown(1) && shooting_debug) {
			actor->setNextOrder(attackOrder(0, (int3)target_pos));
		}
		if((navi_debug || (navi_show && !shooting_debug)) && isMouseKeyDown(1)) {
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
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		world.addToRender(renderer);

		if((entity_debug || shooting_debug) && !isect.isEmpty()) {
			renderer.addBox(isect.boundingBox());
			if(isect.isEntity() && isect.entity()->entityType() == entity_item) {
				Item *item = static_cast<Item*>(isect.entity());
				PTexture tex = item->guiImage(false);
				renderer.add(tex, IRect(int2(0, 0), tex->size()), item->pos() + float3(0, 8, 0), item->boundingBox());
			}
		}

		if(!isect.isEmpty() && shooting_debug) {
			float3 target = ray.at(isect.distance);
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world.intersect(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = shoot_isect.boundingBox();
				renderer.addBox(box, Color::red);
				target_pos = shoot_ray.at(shoot_isect.distance);
			}
		}

		if(navi_debug || navi_show) {
			world.naviMap().visualize(renderer, true);
			world.naviMap().visualizePath(path, 3, renderer);
		}

		renderer.render();
		lookAt(view_pos);

		{
			lookAt({0, 0});
			
			drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
			drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

			gfx::PFont font = gfx::Font::mgr["arial_24"];

			float3 pos = ray.at(isect.distance);
			font->drawShadowed(int2(0, 0), Color::white, Color::black, "(%.2f %.2f %.2f)", pos.x, pos.y, pos.z);

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

