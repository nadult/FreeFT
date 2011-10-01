#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"
#include "tile_map.h"
#include "sys/profiler.h"

using namespace gfx;

namespace
{
	vector<Tile> tiles;

}

int safe_main(int argc, char **argv)
{
	int2 res(1280, 720);
	

	CreateWindow(res, false);
	SetWindowTitle("FT remake version 0.0");

	DTexture tex;
	Loader("../data/epic_boobs.png") & tex;

	const char *mapName = argc > 1? argv[1] : "../data/test.map";

	Font font("../data/fonts/font1.fnt");
	DTexture fontTex; Loader("../data/fonts/font1_00.png") & fontTex;
	SetBlendingMode(bmNormal);

	Sprite spr; {
		Loader loader("../refs/sprites/robots/Behemoth.spr");
	//	Loader loader("../refs/sprites/characters/LeatherMale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
		spr.LoadFromSpr(loader);
	}
	/*
	for(uint n = 0; n < spr.sequences.size(); n++)
		printf("Sequence %s: %d frames\n", spr.sequences[n].name.c_str(), (int)spr.sequences[n].frames.size());
	for(uint n = 0; n < spr.anims.size(); n++)
		printf("Anim %s: %d frames %d dirs; offset: %d\n",
				spr.anims[n].name.c_str(), spr.anims[n].numFrames, spr.anims[n].numDirs, spr.anims[n].offset);
				*/

	int seqId = 0, dirId = 0, frameId = 0, tileId = 0;

	//vector<string> fileNames = FindFiles("../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	vector<string> fileNames = FindFiles("../refs/tiles/Test/", ".til", 1);
	tiles.resize(fileNames.size());

	for(uint n = 0; n < fileNames.size(); n++) {
		Loader(fileNames[n]) & tiles[n];
		tiles[n].name = fileNames[n];
		tiles[n].LoadDTexture();
	}


	DTexture sprTex;
	double lastFrameTime = GetTime();
	bool showSprite = true;
	bool showBBoxes = false;

	int2 oldScreenPos = GetMousePos();

	TileMap tileMap;
	tileMap.Resize({16 * 64, 16 * 64});
	TileMapEditor editor(tileMap);

	IRect view(0, 0, res.x, res.y);
	int3 clickPos(0, 0, 0), worldPos(0, 0, 0);

	double lastSFrameTime = GetTime();
	double sframeTime = 1.0 / 16.0;

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
		int2 screenPos = GetMousePos();
		if(IsMouseKeyPressed(2))
			view -= screenPos - oldScreenPos;
		oldScreenPos = screenPos;
		
		{
			int2 wp(ScreenToWorld(screenPos + view.min));
			worldPos.x = wp.x;
			worldPos.z = wp.y;
		}
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		if(IsKeyDown(Key_pageup)) tileId++;
		if(IsKeyDown(Key_pagedown)) tileId--;
		if(IsKeyDown(Key_kp_add)) worldPos.y++;
		if(IsKeyDown(Key_kp_subtract)) worldPos.y--;
		Clamp(worldPos.y, 0, 255);

		if(IsKeyDown(Key_f5)) {
			string fileName = mapName;
			if(fileName.size())
				Saver(fileName) & tileMap;
		}
		if(IsKeyDown(Key_f8)) {
			string fileName = mapName;
			if(fileName.size()) {
				std::map<string, const gfx::Tile*> dict;
				for(uint n = 0; n < tiles.size(); n++)
					dict[tiles[n].name] = &tiles[n];
				
				try {
					Loader ldr(fileName);
					try { tileMap.Serialize(ldr, &dict); }
					catch(...) {
						tileMap.Clear();
						tileMap.Resize({256, 256});
						editor = TileMapEditor(tileMap);
						throw;
					}
					editor = TileMapEditor(tileMap);
				}
				catch(const Exception &ex) {
					printf("Exception caught: %s\n", ex.What());
				}
			}
		}


		if(IsKeyPressed('T')) g_FloatParam[0] += 0.00001f;
		if(IsKeyPressed('G')) g_FloatParam[0] -= 0.00001f;
		if(IsKeyPressed('Y')) g_FloatParam[1] += 0.00001f;
		if(IsKeyPressed('H')) g_FloatParam[1] -= 0.00001f;
		
		if(IsKeyDown(Key_space)) showSprite ^= 1;
		if(IsKeyDown(Key_f1)) showBBoxes ^= 1;

		tileId += GetMouseWheelMove();

		if(GetTime() - lastSFrameTime > sframeTime) {
			if(lastSFrameTime > sframeTime * 2.0)
				lastSFrameTime = GetTime();
			else
				lastSFrameTime += sframeTime;
			frameId++;
		}

		seqId %= spr.sequences.size();
		tileId %= tiles.size();
			
		tileMap.Render(view, showBBoxes);
	
		if(!showSprite) {
			if(IsMouseKeyDown(1)) {
				try {
					editor.AddTile(tiles[tileId], worldPos);
				}
				catch(const Exception &ex) {
					printf("Exception: %s\n", ex.what());
				}
			}
				
			int3 p1(Min(clickPos.x, worldPos.x), worldPos.y, Min(clickPos.z, worldPos.z));
			int3 p2(Max(clickPos.x, worldPos.x), worldPos.y, Max(clickPos.z, worldPos.z));
			
			if(IsMouseKeyDown(0))
				clickPos = worldPos;
			if(IsMouseKeyUp(0))
				editor.Fill(tiles[tileId], p1, p2);
			if(IsMouseKeyPressed(0))
				DrawBBox(int2(WorldToScreen(p1)) - view.min, p2 - p1);

			editor.DrawPlacingHelpers(view, tiles[tileId], worldPos);
		}

		Sprite::Rect rect;

		if(showSprite) {
			Texture frame = spr.GetFrame(seqId, frameId % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
			sprTex.SetSurface(frame);
			sprTex.Bind();

			int2 size(rect.right - rect.left, rect.bottom - rect.top);
			int2 pos = screenPos;
			DrawQuad(pos.x + rect.left - spr.offset.x, pos.y + rect.top - spr.offset.y, size.x, size.y);

			if(showBBoxes) {
				DTexture::Bind0();
				DrawBBox(pos, spr.bbox);
			}
		}

		char text[256];
		fontTex.Bind();
		font.SetSize(int2(35, 25));

		double time = GetTime();
		double frameTime = time - lastFrameTime;
		lastFrameTime = time;

		string profData = Profiler::GetStats();
		Profiler::NextFrame();

		font.SetPos(int2(0, 0));
		sprintf(text, "Frame time: %.2f ms; %.6f %.6f;  WPos: %d %d\n%s", frameTime * 1000.0f,
				g_FloatParam[0], g_FloatParam[1], worldPos.x, worldPos.y, profData.c_str());
		font.Draw(text);
		
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

