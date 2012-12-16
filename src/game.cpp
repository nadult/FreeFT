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
#include "bvh.h"

using namespace gfx;
using namespace game;

float frand() {
	return float(rand()) / float(RAND_MAX);
}

void bvhTest() {
	BVH<int> tree(FBox(0, 0, 0, 100, 100, 100));
	for(int n = 0; n < 1000000; n++) {
		if(rand() % 2)
			tree.addObject(n, FBox(frand() * 90, frand() * 90, frand() * 90, frand() * 10, frand() * 10, frand() * 10));
		else if(tree.m_objects.size())
			tree.removeObject(rand() % tree.m_objects.size());

	}
	int vis = tree.print();

	int count = 0;
	for(int n = 0; n < (int)tree.m_objects.size(); n++)
		if(tree.m_objects[n].first != -1)
			count++;
	printf("objects: %d nodes: %d visited: %d\n", count, tree.m_nodes.size(), vis);
}


int safe_main(int argc, char **argv)
{
//	bvhTest();
//	return 0;

	Config config = loadConfig("game");
	ItemDesc::loadItems();

	createWindow(config.resolution, config.fullscreen);
	printDeviceInfo();

	setWindowTitle("FTremake ver 0.02");
	grabMouse(false);

	setBlendingMode(bmNormal);

	int2 view_pos(0, 0);

	PFont font = Font::mgr["arial_32"];

	World world("data/tile_map.xml");

	Actor *actor = world.addEntity(new Actor(ActorTypeId::male, float3(100, 1, 70)));

	Container *chest = world.addEntity(new Container("containers/Chest Wooden", float3(134, 1, 37)));
	Container *toolbench = world.addEntity(new Container("containers/Toolbench S", float3(120, 1, 37)));
	Container *fridge = world.addEntity(new Container("containers/Fridge S", float3(134, 1, 25)));
	world.addEntity(new Container("containers/Ice Chest N", float3(120, 1, 25)));

	Door *door = world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 42), Door::type_rotating));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 82), Door::type_rotating, float2(1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(95, 1, 92), Door::type_rotating, float2(-1, 0)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(85, 1, 82), Door::type_rotating, float2(0, 1)));
	world.addEntity(new Door("doors/PWT DOORS/PWT MetalDoor", float3(85, 1, 92), Door::type_rotating, float2(0, -1)));
	world.addEntity(new Door("doors/BOS DOORS/BOS InteriorDoor2", float3(75, 1, 92), Door::type_sliding, float2(0, -1)));
	chest->setDir(float2(0, -1));
	fridge->setDir(float2(-1, 0));
	door->setKey(ItemDesc::find("prison_key"));
	fridge->setKey(ItemDesc::find("prison_key"));

	chest->inventory().add(ItemDesc::find("laser_rifle"), 1);
	chest->inventory().add(ItemDesc::find("plasma_rifle"), 1);
	chest->inventory().add(ItemDesc::find("fusion_cell"), 100);
	chest->inventory().add(ItemDesc::find("prison_key"), 1);
	fridge->inventory().add(ItemDesc::find("power_armour"), 1);
	fridge->inventory().add(ItemDesc::find("m60"), 1);

	world.addEntity(new ItemEntity(ItemDesc::find("leather_armour"), float3(125, 1, 60)));

	world.updateNavigationMap(true);

	printf("Actor size: %.0f %.0f %.0f\n",
			actor->boundingBox().width(),
			actor->boundingBox().height(),
			actor->boundingBox().depth());

	bool navi_show = 0;
	bool navi_debug = 0;
	bool shooting_debug = 1;
	bool entity_debug = 1;
	bool item_debug = 1;
	
	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0);
	float3 target_pos(0, 0, 0);

	const TileMap &tile_map = world.tileMap();

	PTexture atlas; {
		vector<gfx::Tile*> tiles;
		for(int n = 0; n < tile_map.nodeCount(); n++)
			for(int i = 0; i < tile_map(n).instanceCount(); i++)
				tiles.push_back(const_cast<gfx::Tile*>(tile_map(n)(i).m_tile));
		sort(tiles.begin(), tiles.end());
		tiles.resize(unique(tiles.begin(), tiles.end()) - tiles.begin());

		atlas = makeTileAtlas(tiles);
		Saver("atlas.tga") & *atlas;
	}
	int inventory_sel = -1, container_sel = -1;
	string prof_stats;

	while(pollEvents()) {
		double loop_start = getTime();
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();
		
		Ray ray = screenRay(getMousePos() + view_pos);
		Intersection isect = world.pixelIntersect(getMousePos() + view_pos);
		Intersection box_isect = world.intersect(ray, actor);

		if(isMouseKeyDown(0) && !isKeyPressed(Key_lctrl)) {
			if(isect.entity() && entity_debug) {
				//isect.entity->interact(nullptr);
				InteractionMode mode = isect.entity()->entityType() == entity_item? interact_pickup : interact_normal;
				actor->setNextOrder(interactOrder(isect.entity(), mode));
			}
			else if(navi_debug) {
				int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
				world.naviMap().addCollider(IRect(wpos.xz(), wpos.xz() + int2(4, 4)));

			}
			else if(box_isect.isTile()) {
				int3 wpos = (int3)asXZY(ray.at(box_isect.distance).xz(), 1.0f);
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

		double time = getTime();
		if(!navi_debug)
			world.updateNavigationMap(false);

		world.simulate((time - last_time));
		last_time = time;

		clear({128, 64, 0});
		SceneRenderer renderer(IRect(int2(0, 0), config.resolution), view_pos);

		world.addToRender(renderer);

		if(entity_debug && isect.isEntity())
			renderer.addBox(isect.boundingBox(), Color::yellow);

		if(!box_isect.isEmpty() && shooting_debug) {
			float3 target = ray.at(box_isect.distance);
			float3 origin = actor->pos() + ((float3)actor->bboxSize()) * 0.5f;
			float3 dir = target - origin;

			Ray shoot_ray(origin, dir / length(dir));
			Intersection shoot_isect = world.intersect(Segment(shoot_ray, 0.0f), actor);

			if(!shoot_isect.isEmpty()) {
				FBox box = shoot_isect.boundingBox();
				renderer.addBox(box, Color(255, 0, 0, 100));
				target_pos = shoot_ray.at(shoot_isect.distance);
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
		drawQuad(0, 0, 250, 200, Color(0, 0, 0, 80));
		
		gfx::PFont font = gfx::Font::mgr["arial_16"];
		float3 isect_pos = ray.at(box_isect.distance);
		font->drawShadowed(int2(0, 0), Color::white, Color::black, "(%.2f %.2f %.2f)", isect_pos.x, isect_pos.y, isect_pos.z);
		font->drawShadowed(int2(0, 20), Color::white, Color::black, "%s", prof_stats.c_str());

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

			inventory_sel = clamp(inventory_sel, -2, actor->inventory().size() - 1);
			container_sel = clamp(container_sel, 0, container? container->inventory().size() - 1 : 0);

			if(isKeyDown('D') && inventory_sel >= 0)
				actor->setNextOrder(dropItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel >= 0)
				actor->setNextOrder(equipItemOrder(inventory_sel));
			else if(isKeyDown('E') && inventory_sel < 0) {
				InventorySlotId::Type slot_id = InventorySlotId::Type(-inventory_sel - 1);
				actor->setNextOrder(unequipItemOrder(slot_id));
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
		Profiler::updateTimer("main_loop", getTime() - loop_start);
		prof_stats = Profiler::getStats();
		Profiler::nextFrame();
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

