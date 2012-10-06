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

using namespace gfx;

namespace
{
	vector<Tile> tiles;
	Font font;
	DTexture fontTex;
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

float Compare(const Tile &a, const Tile &b) {
	int2 offset = b.offset - a.offset - int2(WorldToScreen(float3(a.bbox.x, 0.0f, 0.0f)));
	int2 aSize = a.texture.Size(), bSize = b.texture.Size();
	int2 size(Min(aSize.x, bSize.x), Min(aSize.y, bSize.y));

//	printf("Size: %dx%d\n", aSize.x, aSize.y);
//	printf("Offset: %dx%d\n", offset.x, offset.y);
	
	int2 min(offset.x < 0? -offset.x : 0, offset.y < 0? -offset.y : 0);
	int2 max(offset.x > 0? size.x - offset.x : size.x, offset.y > 0? size.y - offset.y : size.y);
	
	int missed = 0, matched = 0;
	for(int y = min.y; y < max.y; y++)
		for(int x = min.x; x < max.x; x++) {
			Color aPix = a.texture(x, y);
			Color bPix = b.texture(x + offset.x, y + offset.y);
				
			if(!aPix.a || !bPix.a)
				missed++;
			else {
				int rdist = abs((int)aPix.r - (int)bPix.r);
				int gdist = abs((int)aPix.g - (int)bPix.g);
				int bdist = abs((int)aPix.b - (int)bPix.b);

				int dist = Max(rdist, Max(gdist, bdist));
				if(dist < 20)
					matched++;	
			}
		}

	char text[256];
	snprintf(text, sizeof(text), "size: %dx%d  off: %dx%d  matches: %d/%d", size.x, size.y, offset.x, offset.y,
				matched, (max.x - min.x) * (max.y - min.y) - missed);
	font.Draw(text);

	return float(matched) / float((max.x - min.x) * (max.y - min.y));
}

struct TileSelector {
public:
	TileSelector() :offset(0), tileId(0) { cmpId[0] = cmpId[1] = 0; }

	void loop(int2 res) {
		if(IsKeyPressed(Key_lctrl) || IsMouseKeyPressed(2))
			offset += GetMouseMove().y;
		offset += GetMouseWheelMove() * res.y / 8;
		LookAt({0, 0});
		draw(res);

		LookAt({0, 0});
		fontTex.Bind();
		font.SetSize(int2(35, 25));

		font.SetPos(int2(5, 65));
		Compare(tiles[cmpId[0]], tiles[cmpId[1]]);
	}

	void draw(int2 res) {
		int2 pos(0, offset);
		int maxy = 0;
		int2 mousePos = GetMousePos();

		for(uint n = 0; n < tiles.size(); n++) {
			const Tile &tile = tiles[n];
			IRect bounds = tile.GetBounds();

			if(bounds.Width() + pos.x > res.x && pos.x != 0) {
				pos = int2(0, pos.y + maxy);
				if(pos.y >= res.y)
					break;
				maxy = 0;
			}
			
			if(IRect(pos, pos + bounds.Size()).IsInside(mousePos)) {
				tileId = n;
				if(IsKeyPressed('1'))
					cmpId[0] = n;
				if(IsKeyPressed('2'))
					cmpId[1] = n;
			}

			if(pos.y + bounds.Height() >= 0) {
				tile.Draw(pos - bounds.min);
				DTexture::Bind0();

				if(tileId == (int)n)
					DrawRect(IRect(pos, pos + bounds.Size()));
				if(cmpId[0] == (int)n)
					DrawRect(IRect(pos, pos + bounds.Size()), Color(255, 0, 0));
				if(cmpId[1] == (int)n)
					DrawRect(IRect(pos, pos + bounds.Size()), Color(0, 255, 0));
			}

			pos.x += bounds.Width();
			maxy = Max(maxy, bounds.Height());
		}
	}

	int GetTileId() { return tileId; }

protected:
	int offset;
	int tileId, cmpId[2];
};

int safe_main(int argc, char **argv)
{
	int2 res(1024, 600);

	CreateWindow(res, true);
	SetWindowTitle("FT remake version 0.01");

//	DTexture tex;
//	Loader("../data/epic_boobs.png") & tex;

	//const char *mapName = argc > 1? argv[1] : "../data/test.map";

	font = Font("../data/fonts/font1.fnt");
	Loader("../data/fonts/font1_00.png") & fontTex;
	SetBlendingMode(bmNormal);

	Sprite spr; {
	//	Loader loader("../refs/sprites/robots/Behemoth.spr");
		Loader loader("../refs/sprites/characters/LeatherFemale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
	//	spr.LoadFromSpr(loader);
	}

	int seqId = 0, dirId = 0, frameId = 0;

	//vector<string> fileNames = FindFiles("../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	vector<string> fileNames = FindFiles("../refs/tiles/Mountains/Mountain FLOORS/", ".til", 1);
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

	int2 oldScreenPos = GetMousePos();

	TileMap tile_map;
	tile_map.Resize({16 * 64, 16 * 64});

	TileSelector selector;
	TileMapEditor editor(res);
	editor.setTileMap(&tile_map);

	double lastSFrameTime = GetTime();
	double sframeTime = 1.0 / 16.0;
	bool selectMode = false;

	//TODO: algorytm sprawzajacy ktore tile do siebie pasuja (porownujacy pixele)

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		if(IsKeyDown(Key_space)) selectMode ^= 1;
		
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
		
		int tileId = selector.GetTileId();

		if(GetTime() - lastSFrameTime > sframeTime) {
			if(lastSFrameTime > sframeTime * 2.0)
				lastSFrameTime = GetTime();
			else
				lastSFrameTime += sframeTime;
			frameId++;
		}

		if(!spr.sequences.empty())
			seqId %= spr.sequences.size();
		
		if(selectMode)
			selector.loop(res);
		else
			editor.loop(tileId >= 0 && tileId < tiles.size()? &tiles[tileId] : 0);

		{
			LookAt({0, 0});
			char text[256];
			fontTex.Bind();
			font.SetSize(int2(35, 25));

			double time = GetTime();
			double frameTime = time - lastFrameTime;
			lastFrameTime = time;

			string profData = Profiler::GetStats();
			Profiler::NextFrame();

			font.SetPos(int2(5, 5));
			sprintf(text, "Frame time: %.2f ms; %.6f %.6f;  WPos: %d %d %d\nTile: %s", frameTime * 1000.0f,
					g_FloatParam[0], g_FloatParam[1], 0, 0, 0, /*worldPos.x, worldPos.y, worldPos.z,*/ tiles[tileId].name.c_str());
			font.Draw(text);
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

