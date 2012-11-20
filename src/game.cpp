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

	Actor actor("characters/Power", int3(100, 1, 70));
	printf("Actor size: %d %d %d\n",
			actor.boundingBox().width(),
			actor.boundingBox().height(),
			actor.boundingBox().depth());

	vector<string> file_names;
	findFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	findFiles(file_names, "../refs/tiles/RAIDERS/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Wasteland/", ".til", 1);
//	findFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = findFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = findFiles("../refs/tiles/VAULT/", ".til", 1);

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		Ptr<Tile> tile = Tile::mgr.load(file_names[n]);
		tile->name = file_names[n];
		tile->loadDTexture();
	}
	printf("\n");

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

	
	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0);

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			break;

		if((isKeyPressed(Key_lctrl) && isMouseKeyPressed(0)) || isMouseKeyPressed(2))
			view_pos -= getMouseMove();

		if(isMouseKeyPressed(0) && !isKeyPressed(Key_lctrl)) {
			int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
			actor.setNextOrder(Actor::makeMoveOrder(wpos, isKeyPressed(Key_lshift)));
		}
		if(isMouseKeyDown(1)) {
			int3 wpos = asXZY(screenToWorld(getMousePos() + view_pos), 1);
			path = navigation_map.findPath(last_pos.xz(), wpos.xz());
			last_pos = wpos;
//			printf("Quad: %d\n", navigation_map.findQuad(wpos.xz()));

//			int length = 0;
//			for(int n = 1; n < (int)path.size(); n++)
//				length += abs(path[n].x - path[n - 1].x) + abs(path[n].y - path[n - 1].y);
//			printf("Path length: %d\n", length);
//			for(int n = 0; n < (int)path.size(); n++)
//				printf("(%d %d) ", path[n].x, path[n].y);
//			printf("\n");
		}
		if(isKeyDown(Key_kp_add))
			actor.setNextOrder(Actor::makeChangeStanceOrder(1));
		if(isKeyDown(Key_kp_subtract))
			actor.setNextOrder(Actor::makeChangeStanceOrder(-1));

		double time = getTime();
		actor.think(time, time - last_time); //TODO: problem with delta in the first frame
		last_time = time;

		clear({128, 64, 0});
		SceneRenderer renderer(IRect(0, 0, res.x, res.y), view_pos);

		tile_map.addToRender(renderer);
		actor.addToRender(renderer);

		navigation_map.visualize(renderer, true);
		navigation_map.visualizePath(path, 3, renderer);

		renderer.render();
		lookAt(view_pos);

/*		{
			lookAt({0, 0});
			char text[256];

			double time = GetTime();
			double frameTime = time - lastFrameTime;
			lastFrameTime = time;
			
			string profData = Profiler::getStats();
			Profiler::nextFrame();
			printf("%s\n", profData.c_str());
		}*/

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

