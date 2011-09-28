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
		if(offset.x > 1000 || offset.y > 1000)
			offset = int2(0, 0);
		int2 size = tiles[id].texture.Size();
		DrawQuad(pos.x - offset.x, pos.y - offset.y, size.x, size.y);
	}

	struct Splat {
		int2 pos;
		int tileId;
		
		bool operator<(const Splat &rhs) const {
			return pos.y == rhs.pos.y? pos.x < rhs.pos.x : pos.y < rhs.pos.y;
		}

		void Draw(int2 viewPos) {
			DrawTile(tileId, pos * 6 - viewPos);
		}

		void DrawBBox(int2 viewPos) {
			DTexture::Bind0();
			::DrawBBox(pos * 6 - viewPos, tiles[tileId].bbox);
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
//		Loader loader("../refs/sprites/robots/Behemoth.spr");
		Loader loader("../refs/sprites/characters/LeatherMale.spr");
	//	Loader loader("../refs/sprites/robots/RobotTurrets/PopupTurret.spr");
		spr.LoadFromSpr(loader);
	}
	for(uint n = 0; n < spr.sequences.size(); n++)
		printf("Sequence %s: %d frames\n", spr.sequences[n].name.c_str(), (int)spr.sequences[n].frames.size());
	for(uint n = 0; n < spr.anims.size(); n++)
		printf("Anim %s: %d frames %d dirs; offset: %d\n",
				spr.anims[n].name.c_str(), spr.anims[n].numFrames, spr.anims[n].numDirs, spr.anims[n].offset);


	int seqId = 0, dirId = 0, frameId = 0, tileId = 0;

	//vector<string> fileNames = FindFiles("../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	vector<string> fileNames = FindFiles("../refs/tiles/Test/", ".til", 1);
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

	g_FloatParam[0] = 0.86f;
	g_FloatParam[1] = 0.43f;

	DTexture sprTex;
	double lastFrameTime = GetTime();
	bool showSprite = true;
	bool showBBoxes = false;

	int2 viewPos(0, 0);
	int2 oldScreenPos = GetMousePos();
//	oldScreenPos -= oldScreenPos % 18;

	while(PollEvents()) {
		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
		
		int2 screenPos = GetMousePos();
//		screenPos -= screenPos % 18;
		int2 worldPos = screenPos + viewPos;
		
		if(IsMouseKeyPressed(2))
			viewPos -= screenPos - oldScreenPos;
		oldScreenPos = screenPos;

	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		if(IsKeyDown(Key_pageup)) tileId++;
		if(IsKeyDown(Key_pagedown)) tileId--;

		if(IsKeyPressed('T')) g_FloatParam[0] += 0.0001f;
		if(IsKeyPressed('G')) g_FloatParam[0] -= 0.0001f;
		if(IsKeyPressed('Y')) g_FloatParam[1] += 0.0001f;
		if(IsKeyPressed('H')) g_FloatParam[2] -= 0.0001f;
		
		if(IsKeyDown(Key_space)) showSprite ^= 1;
		if(IsKeyDown(Key_f1)) showBBoxes ^= 1;

		tileId += GetMouseWheelMove();

		frameId++;
		seqId %= spr.sequences.size();
		tileId %= tiles.size();
	
		int2 tileOffset = int2(tiles[tileId].bbox.x, tiles[tileId].bbox.z) * 3;
		int2 tilePos = worldPos + tileOffset;

		Splat newSplat{tilePos / 6, tileId};

		if(IsMouseKeyDown(1)) {
			splats.push_back(newSplat);
			std::sort(splats.begin(), splats.end());
		}
		for(uint n = 0; n < splats.size(); n++)
			splats[n].Draw(viewPos);

		if(showBBoxes)
			for(uint n = 0; n < splats.size(); n++)
				splats[n].DrawBBox(viewPos);
		if(!showSprite) {
			newSplat.Draw(viewPos);
			if(showBBoxes)
				newSplat.DrawBBox(viewPos);
		}

		Sprite::Rect rect;

		if(showSprite) {
			Texture frame = spr.GetFrame(seqId, frameId / 5 % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
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
		sprintf(text, "Frame time: %f ms; %.4f %.4f;  %d %d\n", frameTime * 1000.0f,
				g_FloatParam[0], g_FloatParam[1], worldPos.x / 6, worldPos.y / 6);
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

