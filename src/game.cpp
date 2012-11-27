#include <memory.h>
#include <cstdio>
#include <algorithm>
#include <unistd.h>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"

#include "tile_map.h"
#include "navigation_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include "sys/platform.h"
#include "actor.h"

using namespace gfx;
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

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	setBlendingMode(bmNormal);

	Actor actor("characters/LeatherMale", int3(100, 1, 70));
	printf("Actor size: %d %d %d\n",
			actor.boundingBox().width(),
			actor.boundingBox().height(),
			actor.boundingBox().depth());

	int2 view_pos(0, 0);

	PFont font = Font::mgr["arial_32"];

	TileMap tile_map;

	if(access("../data/tile_map.xml", R_OK) == 0) {
		string text;
		Loader ldr("../data/tile_map.xml");
		text.resize(ldr.size());
		ldr.data(&text[0], ldr.size());
		XMLDocument doc;
		doc.parse<0>(&text[0]); 
		tile_map.loadFromXML(doc);
	}
	
	NavigationMap navigation_map(tile_map.size());
	navigation_map.update(tile_map);
	navigation_map.printInfo();
	actor.m_tile_map = &tile_map;
	actor.m_navigation_map = &navigation_map;
	PTexture tex = navigation_map.getTexture();

	bool navi_debug = false;
	
	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0), target_pos(0, 0, 0);

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();

		if(isMouseKeyPressed(0) && !isKeyPressed(Key_lctrl)) {
			int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
			actor.setNextOrder(moveOrder(wpos, isKeyPressed(Key_lshift)));
		}
		if(isMouseKeyDown(1)) {
			actor.setNextOrder(attackOrder(0, target_pos));
		}
		if(navi_debug && isMouseKeyDown(1)) {
			int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
			path = navigation_map.findPath(last_pos.xz(), wpos.xz());
			last_pos = wpos;
		}
		if(isKeyDown(Key_kp_add))
			actor.setNextOrder(changeStanceOrder(1));
		if(isKeyDown(Key_kp_subtract))
			actor.setNextOrder(changeStanceOrder(-1));
		if(isKeyDown('W'))
			actor.setNextOrder(
				changeWeaponOrder((WeaponClassId::Type)((actor.weaponId() + 1) % WeaponClassId::count)));

		Ray ray = screenRay(getMousePos() + view_pos);
		auto isect = tile_map.intersect(ray, -1.0f/0.0f, 1.0f/0.0f);

		double time = getTime();
		actor.think(time, time - last_time); //TODO: problem with delta in the first frame
		last_time = time;

		clear({128, 64, 0});
		SceneRenderer renderer(IRect(0, 0, res.x, res.y), view_pos);

		tile_map.addToRender(renderer);
		actor.addToRender(renderer);

		if(isect.node_id != -1) {
			IBox box = tile_map(isect.node_id)(isect.instance_id).boundingBox();
			box += tile_map.nodePos(isect.node_id);
			renderer.addBox(box);

			float3 target = ray.at(isect.t);
			float3 origin = actor.pos() + ((float3)actor.bboxSize()) * 0.5f;
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

		if(navi_debug) {
			navigation_map.visualize(renderer, true);
			navigation_map.visualizePath(path, 1, renderer);
		}

		renderer.render();
		lookAt(view_pos);

		{
			lookAt({0, 0});
			char text[256];
			
			drawLine(getMousePos() - int2(5, 0), getMousePos() + int2(5, 0));
			drawLine(getMousePos() - int2(0, 5), getMousePos() + int2(0, 5));

			gfx::PFont font = gfx::Font::mgr["times_24"];

			font->drawShadowed(int2(0, 0), Color::white, Color::black, "(%f %f %f) -> (%f %f %f)",
					ray.origin().x, ray.origin().y, ray.origin().z, ray.dir().x, ray.dir().y, ray.dir().z);
			font->drawShadowed(int2(0, 24), Color::white, Color::black, "%d %d %f",
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

