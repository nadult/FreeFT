#include <memory.h>
#include <cstdio>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"

using namespace gfx;

int safe_main(int argc, char **argv)
{
	int2 res(1280, 720);

	CreateWindow(res, false);
	SetWindowTitle("FT remake version 0.0");

	DTexture tex;
	Loader("../data/epic_boobs.png") & tex;

	DTexture tex1; Loader("../refs/gui/char/big/CORE_prefab5.zar") & tex1;
//	DTexture tex2; Loader("../refs/gui/back/equip.zar") & tex2;

	Font font("../data/fonts/font1.fnt");
	DTexture fontTex; Loader("../data/fonts/font1_00.png") & fontTex;
	SetBlendingMode(bmNormal);

	Sprite spr; {
		Loader loader("../refs/sprites/robots/Behemoth.spr");
		spr.LoadFromSpr(loader);
	}
	for(uint n = 0; n < spr.sequences.size(); n++)
		printf("Sequence %s: %d frames\n", spr.sequences[n].name.c_str(), (int)spr.sequences[n].frames.size());
	for(uint n = 0; n < spr.anims.size(); n++)
		printf("Anim %s: %d frames %d dirs; offset: %d\n",
				spr.anims[n].name.c_str(), spr.anims[n].numFrames, spr.anims[n].numDirs, spr.anims[n].offset);

	uint seqId = 0, dirId = 0, frameId = 0;

	Tile tile; {
		Loader loader("../refs/tiles/Test/Amelia_Airheart/Generic_Object_Metal_AAplaneMain_O1_1_SE.til");
		loader & tile;
	}
	DTexture tex2;
	tex2.SetSurface(tile.texture);

	while(PollEvents()) {
		float frameTime = GetTime();

		if(IsKeyPressed(Key_esc))
			break;

		Clear({128, 64, 0});
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		frameId++;

		tex2.Bind();
		DrawQuad(0, 0, tex2.Width() * 2, tex2.Height() * 2);

		Sprite::Rect rect;

		Texture frame = spr.GetFrame(seqId, frameId / 5 % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId), &rect);
		tex1.SetSurface(frame);
		tex1.Bind();

		rect.left *= 2; rect.right *= 2;
		rect.bottom *= 2; rect.top *= 2;
		int2 size(rect.right - rect.left, rect.bottom - rect.top);
		DrawQuad(100 + rect.left, 100 + rect.top, size.x, size.y);

		fontTex.Bind();
		font.SetPos(int2(400, 100));
		font.SetSize(int2(60, 40));

		{
			char buf[256];
			sprintf(buf, "Rect: %d %d %d %d", rect.left, rect.top, rect.right, rect.bottom);

			//TODO: jakis dziwny bug wstringize jak sie to zapoda
			font.Draw(buf);
		}

		SwapBuffers();
		frameTime = GetTime() - frameTime;

//		enum { desiredFps = 61 };
//		if(frameTime < 1.0 / double(desiredFps))
//			Sleep(1.0 / double(desiredFps) - frameTime);
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

