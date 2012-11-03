#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "tile_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include <fstream>
#include <unistd.h>

using namespace gfx;


void DrawSprite(const gfx::Sprite &spr, int seqId, int frameId, int dirId, int3 position) {
	Sprite::Rect rect;
	Texture frame = spr.GetFrame(seqId, frameId % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
	
	DTexture sprTex;
	sprTex.SetSurface(frame);
	sprTex.Bind();

	int2 size(rect.right - rect.left, rect.bottom - rect.top);
	int2 pos = int2(WorldToScreen(position));
	DrawQuad(pos.x + rect.left - spr.offset.x, pos.y + rect.top - spr.offset.y, size.x, size.y);

//	DTexture::Bind0();
//	DrawBBox(IBox(position, position + spr.bbox));
}

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

	Sprite spr; {
	//	Loader loader("../refs/sprites/robots/Behemoth.spr");
		Loader loader("../refs/sprites/characters/LeatherFemale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
		spr.LoadFromSpr(loader);
	}

	for(int a = 0; a < spr.anims.size(); a++) {
		const Sprite::Animation &anim = spr.anims[a];
		printf("Anim %d: %s (%d*%d frames)\n", a, anim.name.c_str(), (int)anim.images.size() / anim.numDirs, anim.numDirs);
	}
	for(int s = 0; s < spr.sequences.size(); s++) {
		const Sprite::Sequence &seq = spr.sequences[s];
		printf("Seq %d: %s (%d frames, anim: %d)\n", s, seq.name.c_str(), (int)seq.frames.size(), seq.animId);
	}

	int seqId = 0, dirId = 0, frameId = 0;

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

	//double lastFrameTime = GetTime();
	double lastSFrameTime = GetTime();
	double sframeTime = 1.0 / 16.0;

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

	while(PollEvents()) {
		if(IsKeyDown(Key_esc))
			break;

		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up) && dirId < 7) dirId++;
		if(IsKeyDown(Key_down) && dirId > 0) dirId--;
	
		if((IsKeyPressed(Key_lctrl) && IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
			view_pos -= GetMouseMove();

		if(GetTime() - lastSFrameTime > sframeTime) {
			if(lastSFrameTime > sframeTime * 2.0)
				lastSFrameTime = GetTime();
			else
				lastSFrameTime += sframeTime;

			frameId++;
		}

		if(!spr.sequences.empty())
			seqId = (seqId + spr.sequences.size()) % spr.sequences.size();
	
		Clear({128, 64, 0});
		LookAt(view_pos);
		tile_map.render(IRect(int2(0, 0), res));
		DrawSprite(spr, seqId, frameId, dirId, int3(0, 0, 0));

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

