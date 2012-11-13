#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/tile.h"
#include "tile_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include "rapidxml_print.hpp"
#include <fstream>
#include <unistd.h>
#include "ui/window.h"
#include "ui/button.h"
#include "ui/progress_bar.h"
#include "ui/tile_selector.h"
#include "ui/tile_map_editor.h"
#include "ui/tile_group_editor.h"

using namespace gfx;

enum EditorMode {
	emMapEdition,
	emTileGroupEdition,

	emCount,
};

static const char *s_mode_names[] = {
	"Mode: map edition",
	"Mode: tile group edition",
};

class MainWindow: public ui::Window
{
public:
	MainWindow(int2 res) :ui::Window(IRect(0, 0, res.x, res.y), Color::transparent) {
		int left_width = 320;

		m_mode = emMapEdition;
		m_map.resize({16 * 64, 16 * 64});

		load();	

		m_mapper = new ui::TileMapEditor(IRect(left_width, 0, res.x, res.y));
		m_grouper = new ui::TileGroupEditor(IRect(left_width, 0, res.x, res.y));
		m_selector = new ui::TileSelector(IRect(0, 30, left_width, res.y));

		m_mode_button = new ui::Button(IRect(0, 0, left_width * 2 / 3, 30), s_mode_names[m_mode]);
		m_save_button = new ui::Button(IRect(left_width * 2 / 3, 0, left_width, 30), "Save");

		m_selector->setModel(new ui::AllTilesModel);

		m_mapper->setTileMap(&m_map);
		m_mapper->setTileGroup(&m_group);
		m_grouper->setTarget(&m_group);

		ui::PWindow left = new ui::Window(IRect(0, 0, left_width, res.y));
		left->addChild(m_mode_button.get());
		left->addChild(m_save_button.get());
		left->addChild(m_selector.get());

		addChild(std::move(left));
		addChild(m_mapper.get());
		addChild(m_grouper.get());
		m_grouper->setVisible(false);
	}

	virtual void onButtonPressed(ui::Button *button) {
		if(button == m_mode_button) {
			m_mode = (EditorMode)((m_mode + 1) % emCount);
			m_mapper->setVisible(m_mode == emMapEdition);
			m_grouper->setVisible(m_mode == emTileGroupEdition);
			button->setText(s_mode_names[m_mode]);
		}
		else if(button == m_save_button)
			save();
	}

	virtual void handleInput() {
		ui::Window::handleInput();
		m_mapper->setNewTile(m_selector->selection());
	}

	void load() {
		if(access("../data/tile_group.xml", R_OK) == 0) {
			string text;
			Loader ldr("../data/tile_group.xml");
			text.resize(ldr.size());
			ldr.data(&text[0], ldr.size());
			XMLDocument doc;
			doc.parse<0>(&text[0]); 
			m_group.loadFromXML(doc);
		}
		if(access("../data/tile_map.xml", R_OK) == 0) {
			string text;
			Loader ldr("../data/tile_map.xml");
			text.resize(ldr.size());
			ldr.data(&text[0], ldr.size());
			XMLDocument doc;
			doc.parse<0>(&text[0]); 
			m_map.loadFromXML(doc);
		}

	}

	void save() {
		XMLDocument doc;

		if(m_mode == emMapEdition) {
			m_map.saveToXML(doc);
			std::fstream file("../data/tile_map.xml", std::fstream::out);
			printf("Saving tile map\n");
			file << doc;
		}
		else if(m_mode == emTileGroupEdition) {
			m_group.saveToXML(doc);
			std::fstream file("../data/tile_group.xml", std::fstream::out);
			printf("Saving tile group\n");
			file << doc;
		}
	}

	EditorMode	m_mode;

	TileMap		m_map;
	TileGroup	m_group;
	
	ui::PButton	m_mode_button;
	ui::PButton	m_save_button;

	ui::PTileMapEditor		m_mapper;
	ui::PTileGroupEditor	m_grouper;
	ui::PTileSelector		m_selector;
};

int safe_main(int argc, char **argv)
{
#if defined(RES_X) && defined(RES_Y)
	int2 res(RES_X, RES_Y);
#else
	int2 res(1800, 768);
#endif

	createWindow(res, false);
	setWindowTitle("FTremake::editor ver 0.02");
	grabMouse(false);
		
	setBlendingMode(bmNormal);

	vector<string> file_names;
	findFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Snow/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Mountains/Mountain FLOORS/Rock/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Generic tiles/Generic floors/", ".til", 1);
	findFiles(file_names, "../refs/tiles/RAIDERS/", ".til", 1);
	findFiles(file_names, "../refs/tiles/Wasteland/", ".til", 1);
//	findFiles(file_names, "../refs/tiles/VILLAGE/", ".til", 1);
//	findFiles(file_names, "../refs/tiles/Robotic/", ".til", 1);
//	findFiles(file_names, "../refs/tiles/", ".til", 1);
	//vector<string> file_names = findFiles("../refs/tiles/RAIDERS", ".til", 1);
	//vector<string> file_names = findFiles("../refs/tiles/VAULT/", ".til", 1);

	printf("Loading... ");
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		Ptr<Tile> tile = Tile::mgr.load(file_names[n]);
		tile->name = file_names[n];
		tile->loadDTexture();
	}
	printf("\n");

	//double lastFrameTime = getTime();
//	double lastSFrameTime = getTime();
//	double sframeTime = 1.0 / 16.0;

//	PFont font = Font::mgr["font1"];

	MainWindow main_window(res);
	clear({0, 0, 0});

	while(pollEvents()) {
		
	//	if(isKeyPressed('T')) g_FloatParam[0] += 0.00001f;
	//	if(isKeyPressed('G')) g_FloatParam[0] -= 0.00001f;
	//	if(isKeyPressed('Y')) g_FloatParam[1] += 0.00001f;
	//	if(isKeyPressed('H')) g_FloatParam[1] -= 0.00001f;
		
		main_window.handleInput();
		main_window.draw();
		lookAt({0, 0});
	//	int2 mpos = getMousePos();
	//	drawLine(mpos - int2{10, 0}, mpos + int2{10, 0}, Color(255, 255, 255));
	//	drawLine(mpos - int2{0, 10}, mpos + int2{0, 10}, Color(255, 255, 255));
		
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

