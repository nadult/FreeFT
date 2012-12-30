#include <memory.h>
#include <cstdio>
#include <algorithm>
#include <unistd.h>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/tile.h"
#include "tile_map.h"
#include "tile_group.h"
#include "sys/profiler.h"
#include "ui/window.h"
#include "ui/button.h"
#include "ui/progress_bar.h"
#include "ui/text_box.h"
#include "ui/combo_box.h"
#include "editor/tile_selector.h"
#include "editor/tile_map_editor.h"
#include "editor/tile_group_editor.h"
#include "ui/file_dialog.h"
#include "sys/platform.h"
#include "sys/config.h"
#include "sys/xml.h"

using namespace gfx;
using namespace ui;

enum EditorMode {
	editing_tiles,
	editing_tile_groups,

	editing_modes_count,
};

enum TileFilter {
	filter_all,
	filter_floors,
	filter_walls,
	filter_objects,
	filter_other,

	filter_count,
};

static const char *s_filter_names[filter_count] = {
	"all",
	"floors",
	"walls",
	"objects",
	"other",
};

static const char *s_mode_names[editing_modes_count] = {
	"tiles edition",
	"tile groups edition",
};

static const char *s_save_dialog_names[] = {
	"Saving tile map",
	"Saving tile group",
};
static const char *s_load_dialog_names[] = {
	"Loading tile map",
	"Loading tile group",
};

typedef Ptr<TileListModel> PTileListModel;

struct GroupedTilesModel: public TileListModel {
	GroupedTilesModel(const TileGroup &tile_group) {
		tiles.resize(tile_group.groupCount());
		for(int n = 0; n < tile_group.entryCount(); n++) {
			int group_id = tile_group.entryGroup(n);
			if(!tiles[group_id])
				tiles[group_id] = tile_group.entryTile(n);
		}
	}

	int size() const { return (int)tiles.size(); }
	const gfx::Tile* get(int idx, int&) const { return tiles[idx]; }

	vector<const gfx::Tile*> tiles;
};

struct FilteredTilesModel: public TileListModel {
	FilteredTilesModel(PTileListModel model, TileFilter filter) {
		for(int n = 0; n < model->size(); n++)	{
			int group = 0;
			const gfx::Tile *tile = model->get(n, group);
			string name = tile->name;
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);

			bool is_floor = strstr(name.c_str(), "floors/");
			bool is_wall = strstr(name.c_str(), "walls/");
			bool is_object = strstr(name.c_str(), "objects/");

			if(filter == filter_all || (!is_floor && !is_wall && !is_object && filter == filter_other) ||
				(is_floor && filter == filter_floors) ||
				(is_wall && filter == filter_walls) ||
				(is_object && filter == filter_objects))
				m_tiles.push_back(tile);
		}
	}

	int size() const { return (int)m_tiles.size(); }
	const gfx::Tile* get(int idx, int&) const { return m_tiles[idx]; }

protected:
	vector<const gfx::Tile*> m_tiles;
};

class GroupEditorPad: public Window
{
public:
	GroupEditorPad(const IRect &rect, PTileGroupEditor editor, TileGroup *group)
		:Window(rect), m_editor(editor), m_group(group) {
		m_combo_box = new ComboBox(IRect(0, 0, rect.width(), 22), 200, "Filter: ", s_filter_names, filter_count);
		attach(m_combo_box.get());
	}

	PComboBox			m_combo_box;
	TileGroup			*m_group;
	PTileGroupEditor	m_editor;
};

class TilesEditorPad: public Window
{
public:
	TilesEditorPad(const IRect &rect, PTileMapEditor editor, TileGroup *group)
		:Window(rect, Color::transparent), m_editor(editor), m_group(group) {
		int width = rect.width();

		m_filter_box = new ComboBox(IRect(0, 0, width/2, 22), 200, "Filter: ", s_filter_names, filter_count);
		m_filter_box->selectEntry(0);
		m_dirty_bar = new ProgressBar(IRect(width/2, 0, width, 22), true);
		m_selector = new TileSelector(IRect(0, 22, width, rect.height()));
		
		m_selecting_all_tiles = true;
		updateTileList();

		attach(m_filter_box.get());
		attach(m_dirty_bar.get());
		attach(m_selector.get());

		updateDirtyBar();
	}

	TileFilter currentFilter() const {
		TileFilter filter = (TileFilter)m_filter_box->selectedId();
		DASSERT(filter >= 0 && filter < filter_count);
		return filter;
	}

	void updateTileList() {
		PTileListModel model = m_selecting_all_tiles?
			new AllTilesModel : (TileListModel*)new GroupedTilesModel(*m_group);
		model = new FilteredTilesModel(model, currentFilter());
		m_selector->setModel(model);
	}

	void updateDirtyBar() {
		char text[64];
		snprintf(text, sizeof(text), "Dirty tiles: %d%%", (int)(m_dirty_bar->pos() * 100));
		m_dirty_bar->setText(text);
		m_editor->m_dirty_percent = m_dirty_bar->pos();
	}
	
	virtual bool onEvent(const Event &ev) {
		if(ev.type == Event::progress_bar_moved && m_dirty_bar.get() == ev.source)
			updateDirtyBar();
		else if(ev.type == Event::button_clicked && m_editor.get() == ev.source) {
			bool all_tiles =	!(m_editor->m_mode == TileMapEditor::mode_placing_random) &&
								!(m_editor->m_mode == TileMapEditor::mode_filling);
			if(all_tiles != m_selecting_all_tiles) {
				m_selecting_all_tiles = all_tiles;
				updateTileList();
			}
		}
		else if(ev.type == Event::element_selected && m_selector.get() == ev.source) {
			//TODO: print tile name in selector
			//printf("new tile: %s\n", m_selector->selection()? m_selector->selection()->name.c_str() : "none");
			m_editor->setNewTile(m_selector->selection());
		}
		else if(ev.type == Event::element_selected && m_filter_box.get() == ev.source)
			updateTileList();
		else
			return false;

		return true;
	}

	TileGroup		*m_group;

	PTileMapEditor	m_editor;
	PComboBox		m_filter_box;
	PProgressBar 	m_dirty_bar;
	PTileSelector	m_selector;

	bool m_selecting_all_tiles;
};

class EditorWindow: public Window
{
public:
	EditorWindow(int2 res) :Window(IRect(0, 0, res.x, res.y), Color::transparent) {
		int left_width = width() / 5;

		m_mode = editing_tiles;
		m_map.resize({16 * 64, 16 * 64});

		loadTileGroup("data/tile_group.xml");
		loadTileMap("data/tile_map.xml");

		m_tile_editor = new TileMapEditor(IRect(left_width, 0, res.x, res.y));
		m_group_editor = new TileGroupEditor(IRect(left_width, 0, res.x, res.y));

		m_mode_box = new ComboBox(IRect(0, 0, left_width * 1 / 2, 22), 200, "Mode: ", s_mode_names, editing_modes_count);
		m_save_button = new Button(IRect(left_width * 1 / 2, 0, left_width * 3 / 4, 22), "Save");
		m_load_button = new Button(IRect(left_width * 3 / 4, 0, left_width, 22), "Load");

		m_tiles_editor_pad = new TilesEditorPad(IRect(0, 22, left_width, res.y), m_tile_editor, &m_group);
		m_group_editor_pad = new GroupEditorPad(IRect(0, 22, left_width, res.y), m_group_editor, &m_group);

		m_tile_editor->setTileMap(&m_map);
		m_tile_editor->setTileGroup(&m_group);
		m_group_editor->setTarget(&m_group);

		PWindow left = new Window(IRect(0, 0, left_width, res.y), Color::gui_dark);
		left->attach(m_mode_box.get());
		left->attach(m_save_button.get());
		left->attach(m_load_button.get());
		left->attach(m_tiles_editor_pad.get());
		left->attach(m_group_editor_pad.get());

		attach(std::move(left));
		attach(m_tile_editor.get());
		attach(m_group_editor.get());

		m_group_editor->setVisible(false);
		m_group_editor_pad->setVisible(false);
	}

	virtual bool onEvent(const Event &ev) {
		if(ev.source == m_tile_editor.get())
			m_tiles_editor_pad->onEvent(ev);
		else if(ev.type == Event::element_selected && m_mode_box.get() == ev.source) {
			m_mode = (EditorMode)(ev.value);
			m_tile_editor->setVisible(m_mode == editing_tiles);
			m_group_editor->setVisible(m_mode == editing_tile_groups);

			m_tiles_editor_pad->setVisible(m_mode == editing_tiles);
			m_group_editor_pad->setVisible(m_mode == editing_tile_groups);
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
				if(m_mode == editing_tiles)
					saveTileMap(m_file_dialog->path().c_str());
				else
					saveTileGroup(m_file_dialog->path().c_str());
			}
			else if(ev.value && m_file_dialog->mode() == FileDialogMode::opening_file) {
				if(m_mode == editing_tiles)
					loadTileMap(m_file_dialog->path().c_str());
				else
					loadTileGroup(m_file_dialog->path().c_str());

			}

			m_file_dialog = nullptr;
		}
		else
			return false;

		return true;
	}

	void loadTileMap(const char *file_name) {
		printf("Loading TileMap: %s\n", file_name);
		if(access(file_name, R_OK) == 0) {
			XMLDocument doc;
			doc.load(file_name);
			m_map.loadFromXML(doc);
		}
	}


	void loadTileGroup(const char *file_name) {
		printf("Loading TileGroup: %s\n", file_name);
		if(access(file_name, R_OK) == 0) {
			XMLDocument doc;
			doc.load(file_name);
			m_group.loadFromXML(doc);
		}
	}

	void saveTileMap(const char *file_name) const {
		printf("Saving TileMap: %s\n", file_name);
		XMLDocument doc;
		m_map.saveToXML(doc);
		doc.save(file_name);
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

	TileMap		m_map;
	TileGroup	m_group;
	
	PComboBox	m_mode_box;
	PButton		m_save_button;
	PButton		m_load_button;
	PFileDialog m_file_dialog;

	Ptr<GroupEditorPad> m_group_editor_pad;
	Ptr<TilesEditorPad> m_tiles_editor_pad;

	PTileMapEditor		m_tile_editor;
	PTileGroupEditor	m_group_editor;
};


static bool removeSuffix(string &str, const string &suffix) {
	if(str.size() >= suffix.size() && suffix == str.c_str() + str.size() - suffix.size()) {
		str = str.substr(0, str.size() - suffix.size());
		return true;
	}

	return false;
}

int safe_main(int argc, char **argv)
{
	Config config = loadConfig("editor");
	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FTremake::editor ver 0.02");
	grabMouse(false);
		
	setBlendingMode(bmNormal);

	printf("Enumerating tiles\n");
	vector<FileEntry> file_names;
	findFiles(file_names, "refs/tiles/", FindFiles::regular_file | FindFiles::recursive);

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
			if(removeSuffix(tile_name, Tile::mgr.suffix())) {
				Ptr<Tile> tile = Tile::mgr.load(tile_name);
				tile->name = tile_name;
				tile->storeInCache();
			}
		} catch(const Exception &ex) {
			printf("Error: %s\n", ex.what());
		}
	}
	printf("\n");

	EditorWindow main_window(config.resolution);
	clear({0, 0, 0});

	while(pollEvents()) {
		if(isKeyPressed(Key_lalt) && isKeyDown(Key_f4))
			break;
		
		main_window.process();
		main_window.draw();
		lookAt({0, 0});
		
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
	catch(...) { return 1; }
}

