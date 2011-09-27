#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"

using namespace gfx;

namespace
{
	vector<Tile> tiles;
	vector<Ptr<DTexture>> dTiles;

	void DrawTile(int id, int2 pos) {
		dTiles[id]->Bind();
		int2 offset = tiles[id].offset;
		int2 size = tiles[id].texture.Size();
		DrawQuad(pos.x - offset.x, pos.y - offset.y, size.x, size.y);
	}

	struct Splat {
		int2 pos;
		int tileId;
		
		bool operator<(const Splat &rhs) const {
			return pos.y == rhs.pos.y? pos.x < rhs.pos.x : pos.y < rhs.pos.y;
		}
	};

	vector<Splat> splats;

}

int safe_main(int argc, char **argv)
{
	int2 res(1280, 720);

	CreateWindow(res, false);
	SetWindowTitle("FT remake version 0.0");

	DTexture tex;
	Loader("../data/epic_boobs.png") & tex;

	Font font("../data/fonts/font1.fnt");
	DTexture fontTex; Loader("../data/fonts/font1_00.png") & fontTex;
	SetBlendingMode(bmNormal);

	Sprite spr; {
		Loader loader("../refs/sprites/characters/LeatherMale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
		spr.LoadFromSpr(loader);
	}
	for(uint n = 0; n < spr.sequences.size(); n++)
		printf("Sequence %s: %d frames\n", spr.sequences[n].name.c_str(), (int)spr.sequences[n].frames.size());
	for(uint n = 0; n < spr.anims.size(); n++)
		printf("Anim %s: %d frames %d dirs; offset: %d\n",
				spr.anims[n].name.c_str(), spr.anims[n].numFrames, spr.anims[n].numDirs, spr.anims[n].offset);


	uint seqId = 0, dirId = 0, frameId = 0, tileId = 0;

	vector<string> fileNames = FindFiles("../refs/tiles/Generic tiles/Generic floors/Grass/", ".til", 1);
	if(fileNames.size() > 50)
		fileNames.resize(50);
	tiles.resize(fileNames.size());
	dTiles.resize(tiles.size());

	for(uint n = 0; n < fileNames.size(); n++) {
		printf("Loading: %s\n", fileNames[n].c_str());
		Loader(fileNames[n]) & tiles[n];
	}
	for(uint n = 0; n < tiles.size(); n++) {
		dTiles[n] = new DTexture;
		dTiles[n]->SetSurface(tiles[n].texture);
	}

	DTexture sprTex;
	double lastFrameTime = GetTime();
	bool showSprite = true;

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
		int2 mousePos = GetMousePos();
		mousePos.x -= mousePos.x % 18;
		mousePos.y -= mousePos.y % 18;
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		if(IsKeyDown(Key_pageup)) tileId++;
		if(IsKeyDown(Key_pagedown)) tileId--;
		if(IsKeyDown(Key_space)) showSprite ^= 1;

		frameId++;
		seqId %= spr.sequences.size();
		tileId %= tiles.size();

		if(IsMouseKeyDown(1)) {
			splats.push_back(Splat{mousePos, tileId});
			std::sort(splats.begin(), splats.end());
		}
		for(uint n = 0; n < splats.size(); n++)
			DrawTile(splats[n].tileId, splats[n].pos);

		if(!showSprite)
			DrawTile(tileId, mousePos);

		Sprite::Rect rect;

		if(showSprite) {
			Texture frame = spr.GetFrame(seqId, frameId / 5 % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
			sprTex.SetSurface(frame);
			sprTex.Bind();

			int2 size(rect.right - rect.left, rect.bottom - rect.top);
			DrawQuad(mousePos.x + rect.left - spr.offset.x, mousePos.y + rect.top - spr.offset.y, size.x, size.y);
		}

		char text[256];
		fontTex.Bind();
		font.SetPos(int2(400, 0));
		font.SetSize(int2(30, 20));

		if(showSprite)
			sprintf(text, "Rect: %d %d %d %d", rect.left, rect.top, rect.right, rect.bottom);
		else
			sprintf(text, "Pos: %d %d Size: %d %d", tiles[tileId].offset.x, tiles[tileId].offset.y,
						tiles[tileId].texture.Width(), tiles[tileId].texture.Height());
		font.Draw(text);

		double time = GetTime();
		double frameTime = time - lastFrameTime;
		lastFrameTime = time;

		font.SetPos(int2(400, 20));
		sprintf(text, "Frame time: %f ms\n", frameTime * 1000.0f);
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

