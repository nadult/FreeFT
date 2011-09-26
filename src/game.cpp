#include <memory.h>
#include <cstdio>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"

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
	for(int n = 0; n < spr.sequences.size(); n++)
		printf("Sequence %s: %d frames\n", spr.sequences[n].name.c_str(), (int)spr.sequences[n].frames.size());
	for(int n = 0; n < spr.anims.size(); n++)
		printf("Anim %s: %d frames %d dirs; offset: %d\n",
				spr.anims[n].name.c_str(), spr.anims[n].numFrames, spr.anims[n].numDirs, spr.anims[n].offset);

	uint seqId = 0, dirId = 0, frameId = 0;

	while(PollEvents()) {
		float frameTime = GetTime();

		if(IsKeyPressed(Key_esc))
			break;

		Clear({255, 0, 0});
	
		if(IsKeyDown(Key_right)) seqId++;
		if(IsKeyDown(Key_left)) seqId--;
		if(IsKeyDown(Key_up)) dirId++;
		if(IsKeyDown(Key_down)) dirId--;
		frameId++;

		tex1.SetSurface(spr.GetFrame(seqId, frameId / 5 % spr.NumFrames(seqId), dirId % spr.NumDirs(seqId)));
//		printf("Size: %d %d\n", tex1.Width(), tex1.Height());
		tex1.Bind();

		DrawQuad(100, 100, tex1.Width() * 2, tex1.Height() * 2);

		fontTex.Bind();
		font.SetPos(int2(400, 100));
		font.SetSize(int2(60, 40));
		font.Draw("Hello world!");


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

