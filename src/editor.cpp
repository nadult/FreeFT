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
#include "ui/file_dialog.h"

using namespace gfx;
using namespace ui;

enum EditorMode {
	emMapEdition,
	emTileGroupEdition,

	emCount,
};

static const char *s_mode_names[] = {
	"Mode: map edition",
	"Mode: tile group edition",
};

static const char *s_save_dialog_names[] = {
	"Saving tile map",
	"Saving tile group",
};
static const char *s_load_dialog_names[] = {
	"Loading tile map",
	"Loading tile group",
};

class MainWindow: public Window
{
public:
	MainWindow(int2 res) :Window(IRect(0, 0, res.x, res.y), Color::transparent) {
		int left_width = 320;

		m_mode = emMapEdition;
		m_map.resize({16 * 64, 16 * 64});

		loadTileGroup("../data/tile_group.xml");
		loadTileMap("../data/tile_map.xml");

		m_mapper = new TileMapEditor(IRect(left_width, 0, res.x, res.y));
		m_grouper = new TileGroupEditor(IRect(left_width, 0, res.x, res.y));
		m_selector = new TileSelector(IRect(0, 30, left_width, res.y));

		m_mode_button = new Button(IRect(0, 0, left_width * 1 / 2, 30), s_mode_names[m_mode]);
		m_save_button = new Button(IRect(left_width * 1 / 2, 0, left_width * 3 / 4, 30), "Save");
		m_load_button = new Button(IRect(left_width * 3 / 4, 0, left_width, 30), "Load");

		m_selector->setModel(new AllTilesModel);

		m_mapper->setTileMap(&m_map);
		m_mapper->setTileGroup(&m_group);
		m_grouper->setTarget(&m_group);

		PWindow left = new Window(IRect(0, 0, left_width, res.y));
		left->attach(m_mode_button.get());
		left->attach(m_save_button.get());
		left->attach(m_load_button.get());
		left->attach(m_selector.get());

		attach(std::move(left));
		attach(m_mapper.get());
		attach(m_grouper.get());
		m_grouper->setVisible(false);
	}

	virtual bool onEvent(const Event &ev) {
		if(ev.type == Event::button_clicked && m_mode_button == ev.source) {
			m_mode = (EditorMode)((m_mode + 1) % emCount);
			m_mapper->setVisible(m_mode == emMapEdition);
			m_grouper->setVisible(m_mode == emTileGroupEdition);
			m_mode_button->setText(s_mode_names[m_mode]);
		}
		else if(ev.type == Event::button_clicked && m_load_button == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = new FileDialog(dialog_rect, s_load_dialog_names[m_mode], FileDialogMode::opening_file);
			m_file_dialog->setPath("../data/");
			attach(m_file_dialog.get(), true);
		}
		else if(ev.type == Event::button_clicked && m_save_button == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = new FileDialog(dialog_rect, s_save_dialog_names[m_mode], FileDialogMode::saving_file);
			m_file_dialog->setPath("../data/");
			attach(m_file_dialog.get(), true);
		}
		else if(ev.type == Event::element_selected && m_selector == ev.source) {
			//TODO: print tile name in selector
			//printf("new tile: %s\n", m_selector->selection()? m_selector->selection()->name.c_str() : "none");
			m_mapper->setNewTile(m_selector->selection());
		}
		else if(ev.type == Event::window_closed && m_file_dialog == ev.source) {
			if(ev.value && m_file_dialog->mode() == FileDialogMode::saving_file) {
				if(m_mode == emMapEdition)
					saveTileMap(m_file_dialog->path().c_str());
				else
					saveTileGroup(m_file_dialog->path().c_str());
			}
			else if(ev.value && m_file_dialog->mode() == FileDialogMode::opening_file) {
				if(m_mode == emMapEdition)
					loadTileMap(m_file_dialog->path().c_str());
				else
					loadTileGroup(m_file_dialog->path().c_str());

			}

			m_file_dialog = nullptr;
		}
		else return false;

		return true;
	}

	void loadTileMap(const char *file_name) {
		printf("Loading TileMap: %s\n", file_name);
		if(access(file_name, R_OK) == 0) {
			string text;
			Loader ldr(file_name);
			text.resize(ldr.size());
			ldr.data(&text[0], ldr.size());
			XMLDocument doc;
			doc.parse<0>(&text[0]);
			m_map.loadFromXML(doc);
		}
	}


	void loadTileGroup(const char *file_name) {
		printf("Loading TileGroup: %s\n", file_name);
		if(access(file_name, R_OK) == 0) {
			string text;
			Loader ldr(file_name);
			text.resize(ldr.size());
			ldr.data(&text[0], ldr.size());
			XMLDocument doc;
			doc.parse<0>(&text[0]); 
			m_group.loadFromXML(doc);
		}
	}

	void saveTileMap(const char *file_name) const {
		printf("Saving TileMap: %s\n", file_name);
		XMLDocument doc;
		m_map.saveToXML(doc);
		std::fstream file(file_name, std::fstream::out);
		file << doc;
	}

	void saveTileGroup(const char *file_name) const {
		printf("Saving TileGroup: %s\n", file_name);
		XMLDocument doc;
		m_group.saveToXML(doc);
		std::fstream file(file_name, std::fstream::out);
		file << doc;
	}

	EditorMode	m_mode;

	TileMap		m_map;
	TileGroup	m_group;
	
	PButton		m_mode_button;
	PButton		m_save_button;
	PButton		m_load_button;
	PFileDialog m_file_dialog;

	PTileMapEditor		m_mapper;
	PTileGroupEditor	m_grouper;
	PTileSelector		m_selector;
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
		
		main_window.process();
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

