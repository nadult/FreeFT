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
	Font font;
	DTexture fontTex;

}

void DrawGrid(const IBox &box, int2 nodeSize, int y) {
	DTexture::Bind0();

	//TODO: proper drawing when y != 0
	for(int x = box.min.x - box.min.x % nodeSize.x; x <= box.max.x; x += nodeSize.x)
		DrawLine(int3(x, y, box.min.z), int3(x, y, box.max.z), Color(255, 255, 255, 64));
	for(int z = box.min.z - box.min.z % nodeSize.y; z <= box.max.z; z += nodeSize.y)
		DrawLine(int3(box.min.x, y, z), int3(box.max.x, y, z), Color(255, 255, 255, 64));
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

	void Loop(int2 res) {
		if(IsKeyPressed(Key_lctrl) || IsMouseKeyPressed(2))
			offset += GetMouseMove().y;
		offset += GetMouseWheelMove() * res.y / 8;
		LookAt({0, 0});
		Draw(res);

		LookAt({0, 0});
		fontTex.Bind();
		font.SetSize(int2(35, 25));

		font.SetPos(int2(5, 65));
		Compare(tiles[cmpId[0]], tiles[cmpId[1]]);
	}

	void Draw(int2 res) {
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
	bool showGrid = false;

	int2 oldScreenPos = GetMousePos();

	TileMap tileMap;
	tileMap.Resize({16 * 64, 16 * 64});

	IRect view(0, 0, res.x, res.y);
	int3 clickPos(0, 0, 0), worldPos(0, 0, 0);
	int2 gridSize(3, 3);

	TileSelector selector;

	double lastSFrameTime = GetTime();
	double sframeTime = 1.0 / 16.0;
	bool selectMode = false;

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		if(IsKeyDown(Key_kp_add)) worldPos.y++;
		if(IsKeyDown(Key_kp_subtract)) worldPos.y--;
		if(IsKeyDown(Key_space)) selectMode ^= 1;
		
/*		if(IsKeyDown(Key_f5)) {
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
		
		if(IsKeyDown(Key_f2)) {
			if(showGrid) {
				if(gridSize.x == 3)
					gridSize = int2(6, 6);
				else if(gridSize.x == 6)
					gridSize = int2(9, 9);
				else {
					gridSize = int2(3, 3);
					showGrid = false;
				}
			}
			else
				showGrid = true;
		}

		{
			int2 wp(ScreenToWorld(GetMousePos() + view.min - (int2)WorldToScreen(int3(0, worldPos.y, 0))));

			worldPos.x = wp.x;
			worldPos.z = wp.y;
		}
		Clamp(worldPos.y, 0, 255);

		if(showGrid) {
			worldPos.x -= worldPos.x % gridSize.x;
			worldPos.z -= worldPos.z % gridSize.y;
		}

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
		
		if(selectMode) {
			selector.Loop(res);
		}
		else {
			if((IsKeyPressed(Key_lctrl) && !IsMouseKeyPressed(0)) || IsMouseKeyPressed(2))
				view -= GetMouseMove();
			if(IsMouseKeyDown(0))
				clickPos = worldPos;

			LookAt(view.min);

			if(showGrid) {
				int2 p[4] = {
					(int2)ScreenToWorld((float2)view.min),
					(int2)ScreenToWorld((float2)view.max),
					(int2)ScreenToWorld((float2){view.min.x, view.max.y}),
					(int2)ScreenToWorld((float2){view.max.x, view.min.y}) };

				int2 min = Min(Min(p[0], p[1]), Min(p[2], p[3]));
				int2 max = Max(Max(p[0], p[1]), Max(p[2], p[3]));
				IBox box(min.x, 0, min.y, max.x, 0, max.y);
				DrawGrid(box, gridSize, 0);//worldPos.y);
			}
			tileMap.Render(view);

			IBox selection(
				int3(Min(clickPos.x, worldPos.x), Min(clickPos.y, worldPos.y), Min(clickPos.z, worldPos.z)),
				int3(Max(clickPos.x, worldPos.x), Max(clickPos.y, worldPos.y), Max(clickPos.z, worldPos.z)) );
			if(worldPos.y == clickPos.y)
				selection.max.y++;

			if((!IsMouseKeyPressed(0) && !IsMouseKeyUp(0)) || selection.IsEmpty()) {
				selection.min = worldPos;
				selection.max = selection.min + (IsKeyPressed(Key_lshift)?
					int3(tiles[tileId].bbox.x, 1, tiles[tileId].bbox.z) : int3(gridSize.x, 1, gridSize.y));
			}

			if(IsMouseKeyUp(0)) {
				if(IsKeyPressed(Key_lshift))
					tileMap.Fill(tiles[tileId], selection);
				else {
					tileMap.Select(IBox(selection.min, selection.max + int3(0, 1, 0)),
							IsKeyPressed(Key_lctrl)? SelectionMode::add : SelectionMode::normal);
				}
			}
			if(IsKeyPressed(Key_lshift)) {
				tileMap.DrawPlacingHelpers(tiles[tileId], worldPos);
				tileMap.DrawBoxHelpers(IBox(worldPos, worldPos + tiles[tileId].bbox));
			}

			tileMap.DrawBoxHelpers(selection);
			DTexture::Bind0();
			DrawBBox(selection);

			if(IsKeyPressed(Key_del))
				tileMap.DeleteSelected();
		}

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
					g_FloatParam[0], g_FloatParam[1], worldPos.x, worldPos.y, worldPos.z, tiles[tileId].name.c_str());
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

