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
#include "tile_map_editor.h"
#include "tile_selector.h"
#include "tile_group_editor.h"

using namespace gfx;

namespace
{
	vector<Tile> tiles;
}

void DrawSprite(const gfx::Sprite &spr, int seqId, int frameId, int dirId, int3 position) {
	Sprite::Rect rect;
	Texture frame = spr.GetFrame(seqId, frameId % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
	
	DTexture sprTex;
	sprTex.SetSurface(frame);
	sprTex.Bind();

	int2 size(rect.right - rect.left, rect.bottom - rect.top);
	int2 pos = int2(WorldToScreen(position));
	DrawQuad(pos.x + rect.left - spr.offset.x, pos.y + rect.top - spr.offset.y, size.x, size.y);

	DTexture::Bind0();
	DrawBBox(IBox(position, position + spr.bbox));
}

int safe_main(int argc, char **argv)
{
	int2 res(1024, 600);

	CreateWindow(res, false);
	SetWindowTitle("FT remake version 0.01");

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	SetBlendingMode(bmNormal);

	Sprite spr; {
	//	Loader loader("../refs/sprites/robots/Behemoth.spr");
		Loader loader("../refs/sprites/characters/LeatherFemale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
	//	spr.LoadFromSpr(loader);
	}

	int seqId = 0, dirId = 0, frameId = 0;

	//vector<string> fileNames = FindFiles("../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	vector<string> fileNames = FindFiles("../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	//vector<string> fileNames = FindFiles("../refs/tiles/VAULT/", ".til", 1);
	tiles.resize(fileNames.size());

	printf("Loading... ");
	for(uint n = 0; n < fileNames.size(); n++) {
		if(n * 100 / tiles.size() > (n - 1) * 100 / tiles.size()) {
			printf(".");
			fflush(stdout);
		}

		try {
			Loader(fileNames[n]) & tiles[n];
		}
		catch(...) {
			tiles[n] = Tile();
		}
		tiles[n].name = fileNames[n];
		tiles[n].LoadDTexture();
	}
	printf("\n");

	double lastFrameTime = GetTime();

	TileMap tile_map;
	tile_map.Resize({16 * 64, 16 * 64});

	TileGroup tile_group;

	TileSelector selector(res);
	TileMapEditor editor(res);
	TileGroupEditor group_editor(res);

	editor.setTileMap(&tile_map);
	group_editor.setSource(&tiles);
	group_editor.setTarget(&tile_group);
	selector.setSource(&tiles);

	double lastSFrameTime = GetTime();
	double sframeTime = 1.0 / 16.0;

	enum {
		mTileMapEditor,
		mTileSelector,
		mTileGroupEditor,
	} mode = mTileMapEditor;

	const char *mode_name[] = {
		"TileMap editor",
		"Tile selector",
		"TileGroup editor",
	};

	PFont font = Font::mgr["font1"];
	PTexture fontTex = Font::tex_mgr["font1"];

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;

		if(IsKeyDown(Key_f1) || (mode == mTileSelector && IsKeyDown(Key_space)))
			mode = mTileMapEditor;
		else if(IsKeyDown(Key_f2) || (mode == mTileMapEditor && IsKeyDown(Key_space)))
			mode = mTileSelector;
		else if(IsKeyDown(Key_f3))
			mode = mTileGroupEditor;
		
/*		if(IsKeyDown(Key_f5)) {
			string fileName = mapName;
			if(fileName.size())
				Saver(fileName) & tile_map;
		}
		if(IsKeyDown(Key_f8)) {
			string fileName = mapName;
			if(fileName.size()) {
				std::map<string, const gfx::Tile*> dict;
				for(uint n = 0; n < tiles.size(); n++)
					dict[tiles[n].name] = &tiles[n];
				
				try {
					Loader ldr(fileName);
					try { tile_map.Serialize(ldr, &dict); }
					catch(...) {
						tile_map.Clear();
						tile_map.Resize({256, 256});
						throw;
					}
				}
				catch(const Exception &ex) {
					printf("Exception caught: %s\n", ex.What());
				}
			}
		} */

	//	if(IsKeyPressed('T')) g_FloatParam[0] += 0.00001f;
	//	if(IsKeyPressed('G')) g_FloatParam[0] -= 0.00001f;
	//	if(IsKeyPressed('Y')) g_FloatParam[1] += 0.00001f;
	//	if(IsKeyPressed('H')) g_FloatParam[1] -= 0.00001f;
		
		int tile_id = selector.tileId();

		if(GetTime() - lastSFrameTime > sframeTime) {
			if(lastSFrameTime > sframeTime * 2.0)
				lastSFrameTime = GetTime();
			else
				lastSFrameTime += sframeTime;
			frameId++;
		}

		if(!spr.sequences.empty())
			seqId %= spr.sequences.size();
	
		if(mode == mTileMapEditor)
			editor.loop(tile_id >= 0 && tile_id < tiles.size()? &tiles[tile_id] : 0);
		else if(mode == mTileSelector)
			selector.loop();
		else if( mode == mTileGroupEditor)
			group_editor.loop();

		{
			LookAt({0, 0});
			char text[256];
			fontTex->Bind();

			double time = GetTime();
			double frameTime = time - lastFrameTime;
			lastFrameTime = time;

			string profData = Profiler::GetStats();
			Profiler::NextFrame();

			font->SetSize(int2(25, 18));
			font->SetPos(int2(5, 5));

			sprintf(text, "%s", mode == mTileGroupEditor? group_editor.title() : mode_name[mode]);

			font->Draw(text);
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
		printf("%s\n\nBacktrace:\n%s\n", ex.What(), CppFilterBacktrace(ex.Backtrace()).c_str());
		return 1;
	}
}

