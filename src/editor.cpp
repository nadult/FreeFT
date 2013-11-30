/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>
#include <algorithm>
#include <unistd.h>

#include "gfx/device.h"
#include "gfx/font.h"
#include "game/tile_map.h"
#include "game/tile.h"
#include "game/entity_map.h"
#include "game/level.h"
#include "game/item.h"
#include "sys/profiler.h"
#include "ui/window.h"
#include "ui/button.h"
#include "ui/progress_bar.h"
#include "ui/text_box.h"
#include "ui/combo_box.h"
#include "editor/tile_selector.h"
#include "editor/tiles_editor.h"
#include "editor/group_editor.h"
#include "editor/entities_editor.h"
#include "editor/tiles_pad.h"
#include "editor/entities_pad.h"
#include "editor/group_pad.h"

#include "editor/view.h"
#include "ui/file_dialog.h"
#include "sys/platform.h"
#include "sys/config.h"
#include "sys/xml.h"

using namespace gfx;
using namespace ui;
using namespace game;

enum EditorMode {
	editing_tiles,
	editing_entities,
	editing_group,

	editing_modes_count,
};

static const char *s_mode_names[editing_modes_count] = {
	"tile map edition",
	"entities edition",
	"tile group edition",
};

static const char *s_save_dialog_names[] = {
	"Saving map",
	"Saving map",
	"Saving tile groups",
};
static const char *s_load_dialog_names[] = {
	"Loading map",
	"Loading map",
	"Loading tile groups",
};

class EditorWindow: public Window
{
public:
	EditorWindow(int2 res) :Window(IRect(0, 0, res.x, res.y), Color::transparent), m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map) {
		m_left_width = width() / 5;

		m_mode = editing_tiles;
		m_tile_map.resize(int2(1024, 1024));
		m_entity_map.resize(int2(1024, 1024));

		loadTileGroup("data/tile_group.xml");
		m_group_editor = new GroupEditor(IRect(m_left_width, 0, res.x, res.y));
		m_group_editor->setTarget(&m_group);

		m_mode_box = new ComboBox(IRect(0, 0, m_left_width * 1 / 2, 22), 0,
				"Mode: ", s_mode_names, editing_modes_count);
		m_save_button = new Button(IRect(m_left_width * 1 / 2, 0, m_left_width * 3 / 4, 22), "Save");
		m_load_button = new Button(IRect(m_left_width * 3 / 4, 0, m_left_width, 22), "Load");

		m_group_pad = new GroupPad(IRect(0, 22, m_left_width, res.y), m_group_editor, &m_group);

		m_left_window = new Window(IRect(0, 0, m_left_width, res.y), Color::gui_dark);
		m_left_window->attach(m_mode_box.get());
		m_left_window->attach(m_save_button.get());
		m_left_window->attach(m_load_button.get());
		m_left_window->attach(m_group_pad.get());

		attach(m_left_window.get());
		attach(m_group_editor.get());

		loadMap("data/maps/mission05_mod.xml");

		recreateEditors();
	}

	void freeEditors() {
		if(m_tiles_editor) {
			DASSERT(m_left_window);
			m_left_window->detach((PWindow)m_tiles_pad.get());
			m_left_window->detach((PWindow)m_entities_pad.get());
			detach((PWindow)m_tiles_editor.get());
			detach((PWindow)m_entities_editor.get());

			m_tiles_editor = nullptr;
			m_entities_editor = nullptr;
			m_tiles_pad = nullptr;
			m_entities_pad = nullptr;
		}
	}

	void recreateEditors() {
		freeEditors();

		IRect rect(m_left_width, 0, width(), height());
		m_view = PView(new View(m_tile_map, rect.size()));
		m_tiles_editor = new TilesEditor(m_tile_map, *m_view.get(), rect);
		m_entities_editor = new EntitiesEditor(m_tile_map, m_entity_map, *m_view.get(), rect);
		m_tiles_editor->setTileGroup(&m_group);

		m_tiles_pad = new TilesPad(IRect(0, 22, m_left_width, height()), m_tiles_editor, &m_group);
		m_entities_pad = new EntitiesPad(IRect(0, 22, m_left_width, height()), m_entities_editor);
		
		attach(m_tiles_editor.get());
		attach(m_entities_editor.get());
		m_left_window->attach(m_tiles_pad.get());
		m_left_window->attach(m_entities_pad.get());
		
		updateVisibility();
	}

	void updateVisibility() {
		m_entities_editor->setVisible(m_mode == editing_entities);
		m_tiles_editor->setVisible(m_mode == editing_tiles);
		m_group_editor->setVisible(m_mode == editing_group);

		m_tiles_pad->setVisible(m_mode == editing_tiles);
		m_entities_pad->setVisible(m_mode == editing_entities);
		m_group_pad->setVisible(m_mode == editing_group);
	}

	virtual bool onEvent(const Event &ev) {
		if(ev.source == m_tiles_editor.get())
			m_tiles_pad->onEvent(ev);
		else if(ev.source == m_entities_editor.get())
			m_entities_pad->onEvent(ev);
		else if(ev.type == Event::element_selected && m_mode_box.get() == ev.source) {
			m_mode = (EditorMode)(ev.value);
			updateVisibility();
		}
		else if(ev.type == Event::button_clicked && m_load_button.get() == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = new FileDialog(dialog_rect, s_load_dialog_names[m_mode], FileDialogMode::opening_file);
			m_file_dialog->setPath("data/");
			attach(m_file_dialog.get(), true);
		}
		else if(ev.type == Event::button_clicked && m_save_button.get() == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = new FileDialog(dialog_rect, s_save_dialog_names[m_mode], FileDialogMode::saving_file);
			m_file_dialog->setPath("data/");
			attach(m_file_dialog.get(), true);
		}
		else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			if(ev.value && m_file_dialog->mode() == FileDialogMode::saving_file) {
				if(m_mode == editing_tiles || m_mode == editing_entities)
					saveMap(m_file_dialog->path().c_str());
				else
					saveTileGroup(m_file_dialog->path().c_str());
			}
			else if(ev.value && m_file_dialog->mode() == FileDialogMode::opening_file) {
				if(m_mode == editing_tiles || m_mode == editing_entities)
					loadMap(m_file_dialog->path().c_str());
				else
					loadTileGroup(m_file_dialog->path().c_str());

			}

			m_file_dialog = nullptr;
		}
		else
			return false;

		return true;
	}

	void loadMap(const char *file_name) {
		printf("Loading Map: %s ", file_name);
		fflush(stdout);
		double time = getTime();

		if(access(file_name, R_OK) == 0) {
			m_level.load(file_name);
			recreateEditors();
		}
		printf("(%.2f sec)\n", getTime() - time);
	}


	void loadTileGroup(const char *file_name) {
		printf("Loading TileGroup: %s\n", file_name);
		if(access(file_name, R_OK) == 0) {
			XMLDocument doc;
			doc.load(file_name);
			m_group.loadFromXML(doc);
		}
	}

	void saveMap(const char *file_name) const {
		printf("Saving Map: %s ", file_name);
		fflush(stdout);
		double time = getTime();
		m_level.save(file_name);
		printf(" (%.2f sec)\n", getTime() - time);
		//TODO: nie ma warninga ze nie udalo sie zapisac
	}

	void saveTileGroup(const char *file_name) const {
		printf("Saving TileGroup: %s\n", file_name);
		XMLDocument doc;
		m_group.saveToXML(doc);
		doc.save(file_name);
		//TODO: nie ma warninga ze nie udalo sie zapisac
	}

	EditorMode	m_mode;

	Level		m_level;
	TileMap		&m_tile_map;
	EntityMap	&m_entity_map;
	TileGroup	m_group;
	
	PComboBox	m_mode_box;
	PButton		m_save_button;
	PButton		m_load_button;
	PFileDialog m_file_dialog;

	PWindow			m_left_window;
	PGroupPad		m_group_pad;
	PTilesPad		m_tiles_pad;
	PEntitiesPad	m_entities_pad;

	PView			m_view;
	PTilesEditor	m_tiles_editor;
	PEntitiesEditor	m_entities_editor;
	PGroupEditor	m_group_editor;

	int m_left_width;
};

int safe_main(int argc, char **argv)
{
	Config config = loadConfig("editor");
	game::ItemDesc::loadItems();

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::editor; built " __DATE__ " " __TIME__);
	grabMouse(false);
		
	setBlendingMode(bmNormal);

	printf("Enumerating tiles\n");
	vector<FileEntry> file_names;
	findFiles(file_names, "data/tiles/", FindFiles::regular_file | FindFiles::recursive);

	printf("Loading tiles");
	Path tiles_path = Path(Tile::mgr.prefix()).absolute();
	for(uint n = 0; n < file_names.size(); n++) {
		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		try {
			Path tile_path = file_names[n].path.relative(tiles_path);
			string tile_name = tile_path;
			if(removeSuffix(tile_name, Tile::mgr.suffix()))
				Ptr<Tile> tile = Tile::mgr.load(tile_name);
		} catch(const Exception &ex) {
			printf("Error: %s\n", ex.what());
		}
	}
	printf("\n");

	EditorWindow main_window(config.resolution);
	clear(Color(0, 0, 0));
	string prof_stats;
	double stat_update_time = getTime();
	double start_time = getTime();

	while(pollEvents()) {
		double loop_start = profiler::getTime();
		if(isKeyPressed(Key_lalt) && isKeyDown(Key_f4))
			break;
		
		Tile::setFrameCounter((int)((getTime() - start_time) * 15.0));

		main_window.process();
		main_window.draw();
		lookAt({0, 0});

		if(config.profiler_enabled) {
			DTexture::bind0();
			drawQuad(config.resolution - int2(280, 200), config.resolution, Color(0, 0, 0, 80));

			gfx::PFont font = gfx::Font::mgr["liberation_16"];
			font->drawShadowed(config.resolution - int2(280, 180), Color::white, Color::black, "%s", prof_stats.c_str());
		}

		swapBuffers();
		TextureCache::main_cache.nextFrame();

		profiler::updateTimer("main_loop", profiler::getTime() - loop_start);
		if(getTime() - stat_update_time > 0.25) {
			prof_stats = profiler::getStats();
			stat_update_time = getTime();
		}
		profiler::nextFrame();
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
	catch(...) { return 1; }
}

