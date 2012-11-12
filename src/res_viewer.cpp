#include <memory.h>
#include <cstdio>
#include <algorithm>
#include <unistd.h>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"

using namespace gfx;

int safe_main(int argc, char **argv)
{
#if defined(RES_X) && defined(RES_Y)
	int2 res(RES_X, RES_Y);
#else
	int2 res(1400, 768);
#endif

	createWindow(res, false);
	setWindowTitle("FTremake::Resource Viewer");
	grabMouse(false);

	setBlendingMode(bmNormal);

	PFont font = Font::mgr["arial_32"];
	
	vector<string> tex_files;
	findFiles(tex_files, "../refs/gui/", ".zar", true);
	int tex_id = 0;

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			break;

		clear({128, 64, 0});

		if(!tex_files.empty()) {
			static int t = 0; t++;
			if(t % 8 == 0) {
				if(isKeyPressed(Key_pageup)) tex_id++;
				if(isKeyPressed(Key_pagedown)) tex_id--;
			}
			tex_id = (tex_id + tex_files.size()) % tex_files.size();

			DTexture tex;
			Loader(tex_files[tex_id]) & tex;
			tex.bind();

			int2 pos = res / 2 - tex.size() / 2;
			drawQuad(pos, tex.size());
		}

		lookAt({0, 0});
		char text[256];

		if(!tex_files.empty()) {
			sprintf(text, "tex: %s\n", tex_files[tex_id].c_str());
			font->drawShadowed(int2(0, 0), Color::white, Color::black, text);
		}

		swapBuffers();
	}

	destroyWindow();

	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		destroyWindow();
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

