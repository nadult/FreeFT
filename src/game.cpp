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

	CreateWindow(res, false);
	SetWindowTitle("FTremake ver 0.02");
	GrabMouse(false);

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	SetBlendingMode(bmNormal);

	Actor actor("characters/Power", int3(100, 1, 70));
	printf("Actor size: %d %d %d\n",
			actor.boundingBox().Width(),
			actor.boundingBox().Height(),
			actor.boundingBox().Depth());

	vector<string> file_names;
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/RAIDERS/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Wasteland/", ".til", 1);
//	FindFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/VAULT/", ".til", 1);

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		Ptr<Tile> tile = Tile::mgr.load(file_names[n]);
		tile->name = file_names[n];
		tile->LoadDTexture();
	}
	printf("\n");

	int2 view_pos(0, 0);

	PFont font = Font::mgr["font1"];
	PTexture fontTex = Font::tex_mgr["font1"];

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
	
	NavigationMap navigation_map(tile_map.size() / 2);
	navigation_map.update(tile_map);
	navigation_map.printInfo();
	actor.m_tile_map = &tile_map;
	actor.m_navigation_map = &navigation_map;
	PTexture tex = navigation_map.getTexture();

	double last_time = getTime();
	vector<int2> path;
	int3 last_pos(0, 0, 0);

	while(PollEvents()) {
		if(IsKeyDown(Key_esc))
			break;

		if((IsKeyPressed(Key_lctrl) && IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
			view_pos -= GetMouseMove();

		if(IsMouseKeyPressed(0) && !IsKeyPressed(Key_lctrl)) {
			int3 wpos = AsXZY(ScreenToWorld(GetMousePos() + view_pos), 1);
			actor.setNextOrder(Actor::makeMoveOrder(wpos, IsKeyPressed(Key_lshift)));
		}
		if(IsMouseKeyUp(1)) {
			int3 wpos = AsXZY(ScreenToWorld(GetMousePos() + view_pos), 1);
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
		if(IsKeyDown(Key_kp_add))
			actor.setNextOrder(Actor::makeChangeStanceOrder(1));
		if(IsKeyDown(Key_kp_subtract))
			actor.setNextOrder(Actor::makeChangeStanceOrder(-1));

		double time = getTime();
		actor.think(time, time - last_time); //TODO: problem with delta in the first frame
		last_time = time;

		Clear({128, 64, 0});
		SceneRenderer renderer(IRect(0, 0, res.x, res.y), view_pos);

		tile_map.addToRender(renderer);
		actor.addToRender(renderer);
		navigation_map.visualize(renderer, true);
		for(int n = 1; n < (int)path.size(); n++) {
			int2 begin = path[n - 1], end = path[n];

			int2 pos = begin;
			while(pos.x != end.x && pos.y != end.y) {
				renderer.addBox(IBox(pos.x, 1, pos.y, pos.x + 3, 1, pos.y + 3));
				pos.x += end.x > pos.x? 1 : -1;
				pos.y += end.y > pos.y? 1 : -1;
			}
			while(pos.x != end.x) {
				renderer.addBox(IBox(pos.x, 1, pos.y, pos.x + 3, 1, pos.y + 3));
				pos.x += end.x > pos.x? 1 : -1;
			}
			while(pos.y != end.y) {
				renderer.addBox(IBox(pos.x, 1, pos.y, pos.x + 3, 1, pos.y + 3));
				pos.y += end.y > pos.y? 1 : -1;
			}
		}
		renderer.render();

//		tex->Bind();
//		LookAt(int2(0, 0));
//		DrawQuad(0, 0, 256, 256);

		{
			LookAt({0, 0});
			char text[256];
			fontTex->Bind();

			//double time = GetTime();
			//double frameTime = time - lastFrameTime;
			//lastFrameTime = time;
			
			string profData = Profiler::GetStats();
			Profiler::NextFrame();
//			printf("%s\n", profData.c_str());
		}

		SwapBuffers();
	}

	DestroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		DestroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

