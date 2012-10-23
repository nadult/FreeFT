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
#include "rapidxml_print.hpp"
#include <fstream>
#include <unistd.h>
#include "ui/window.h"
#include "ui/button.h"

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
	int2 res(960, 1000);

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

	vector<string> file_names;
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	FindFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
//	FindFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = FindFiles("../refs/tiles/VAULT/", ".til", 1);
	tiles.resize(file_names.size());

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / tiles.size() > (n - 1) * 100 / tiles.size()) {
			printf(".");
			fflush(stdout);
		}

		try {
			Loader(file_names[n]) & tiles[n];
		}
		catch(...) {
			tiles[n] = Tile();
		}
		tiles[n].name = file_names[n];
		tiles[n].LoadDTexture();
	}
	printf("\n");

	//double lastFrameTime = GetTime();

	TileMap tile_map;
	tile_map.Resize({16 * 64, 16 * 64});

	FloorTileGroup tile_group;
/*	if(access("tile_group.xml", R_OK) == 0) {
		string text;
		Loader ldr("tile_group.xml");
		text.resize(ldr.Size());
		ldr.Data(&text[0], ldr.Size());
		XMLDocument doc;
		doc.parse<0>(&text[0]); 
		tile_group.loadFromXML(doc, tiles);
	}*/

	TileMapEditor editor(res);
	FloorTileGroupEditor group_editor(res);

	editor.setTileMap(&tile_map);
	group_editor.setSource(&tiles);
	group_editor.setTarget(&tile_group);
	editor.setTileGroup(&tile_group);

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
		"FloorTileGroup editor",
	};

	PFont font = Font::mgr["font1"];
	PTexture fontTex = Font::tex_mgr["font1"];

	ui::Window main_window(IRect{0, 0, res.x, res.y}, Color(0, 0, 0, 0)); {
		int left_width = 320;

		ui::Window *left  = new ui::Window(IRect(0, 0, left_width, res.y), Color(255, 0, 0));
		ui::Window *right = new ui::Window(IRect(left_width, 0, res.x, res.y), Color(0, 255, 0));
		TileSelector *selector = new TileSelector(IRect(0, 30, left_width, res.y));
		selector->setSource(&tiles);

		left ->addChild((ui::PWindow)new ui::Button(IRect(0, 0, left_width, 30), "Test Button #1"));
		left ->addChild((ui::PWindow)selector);
		right->addChild((ui::PWindow)new ui::Button(IRect(25, 0, left_width + 25, 30), "Test Button #2"));

		main_window.addChild((ui::PWindow)left);
		main_window.addChild((ui::PWindow)right);
	}

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
		
		int tile_id = 0;//selector.tileId();

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
			editor.loop(tile_id >= 0 && tile_id < (int)tiles.size()? &tiles[tile_id] : 0);
	//	else if(mode == mTileSelector)
	//		selector.loop();
		else if( mode == mTileGroupEditor)
			group_editor.loop();

		main_window.handleInput();
		main_window.draw();

		{
			LookAt({0, 0});
			char text[256];
			fontTex->Bind();

			//double time = GetTime();
			//double frameTime = time - lastFrameTime;
			//lastFrameTime = time;
			
			string profData = Profiler::GetStats();
			Profiler::NextFrame();

		//	font->SetSize(int2(25, 18));
		//	font->SetPos(int2(5, 5));

		//	sprintf(text, "%s", mode == mTileGroupEditor? group_editor.title() : mode_name[mode]);

		//	font->Draw(text);
		}
		
		SwapBuffers();
	}

/*	{
		XMLDocument doc;
		tile_group.saveToXML(doc);

		std::fstream file("tile_group.xml", std::fstream::out);
		file << doc;
	} */

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

