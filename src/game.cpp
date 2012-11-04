#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"

#include "tile_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include "actor.h"

using namespace gfx;

int safe_main(int argc, char **argv)
{
	int2 res(1400, 768);

	CreateWindow(res, false);
	SetWindowTitle("FTremake ver 0.02");
	GrabMouse(false);

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	SetBlendingMode(bmNormal);

	Actor actor("characters/LeatherFemale", int3(100, 1, 70));

	vector<string> file_names;
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/RAIDERS/", ".til", 1);
//	FindFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/VAULT/", ".til", 1);

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		Ptr<Tile> tile = Tile::mgr.Load(file_names[n]);
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
		text.resize(ldr.Size());
		ldr.Data(&text[0], ldr.Size());
		XMLDocument doc;
		doc.parse<0>(&text[0]); 
		tile_map.loadFromXML(doc);
	}

	double last_time = GetTime();

	while(PollEvents()) {
		if(IsKeyDown(Key_esc))
			break;

		if((IsKeyPressed(Key_lctrl) && IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
			view_pos -= GetMouseMove();

		if(IsMouseKeyPressed(0) && !IsKeyPressed(Key_lctrl)) {
			int3 wpos = AsXZY(ScreenToWorld(GetMousePos() + view_pos), 1);
			actor.setNextOrder(Actor::makeMoveOrder(wpos, IsKeyPressed(Key_lshift)));
		}
		if(IsKeyDown(Key_kp_add))
			actor.setNextOrder(Actor::makeChangeStanceOrder(1));
		if(IsKeyDown(Key_kp_subtract))
			actor.setNextOrder(Actor::makeChangeStanceOrder(-1));

		double time = GetTime();
		actor.think(time, time - last_time); //TODO: problem with delta in the first frame
		last_time = time;

		Clear({128, 64, 0});
		SceneRenderer renderer(IRect(0, 0, res.x, res.y), view_pos);

		tile_map.addToRender(renderer);
		actor.addToRender(renderer);

		renderer.render();

		/*{
			LookAt({0, 0});
			char text[256];
			fontTex->Bind();

			//double time = GetTime();
			//double frameTime = time - lastFrameTime;
			//lastFrameTime = time;
			
			string profData = Profiler::GetStats();
			Profiler::NextFrame();
		}*/

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
		printf("%s\n\nBacktrace:\n%s\n", ex.What(), CppFilterBacktrace(ex.Backtrace()).c_str());
		return 1;
	}
}

