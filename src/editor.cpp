// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/entities_editor.h"
#include "editor/entities_pad.h"
#include "editor/group_editor.h"
#include "editor/group_pad.h"
#include "editor/tile_selector.h"
#include "editor/tiles_editor.h"
#include "editor/tiles_pad.h"
#include "editor/view.h"

#include "game/entity_map.h"
#include "game/item.h"
#include "game/level.h"
#include "game/tile.h"
#include "game/tile_map.h"

#include "ui/button.h"
#include "ui/combo_box.h"
#include "ui/file_dialog.h"
#include "ui/progress_bar.h"
#include "ui/text_box.h"
#include "ui/window.h"

#include "res_manager.h"
#include "sys/config.h"

#include "sys/gfx_device.h"

#include <fwk/sys/on_fail.h>
#include <fwk/vulkan/vulkan_window.h>

#include <fwk/libs_msvc.h>
#include <fwk/vulkan/vulkan_window.h>

#ifdef FWK_PLATFORM_WINDOWS
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "mpg123.lib")
#pragma comment(lib, "OpenAL32.lib")
#endif

using namespace ui;
using game::EntityMap;
using game::Level;
using game::Sprite;
using game::Tile;
using game::TileMap;

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

class EditorWindow : public Window {
  public:
	EditorWindow(GfxDevice &gfx_device)
		: Window(IRect(gfx_device.window_ref->size()), ColorId::transparent),
		  m_gfx_device(gfx_device), m_tile_map(m_level.tile_map), m_entity_map(m_level.entity_map) {
		m_maps_path = FilePath(ResManager::instance().dataPath()) / "maps";
		m_left_width = width() / 5;
		int2 res = gfx_device.window_ref->size();

		m_mode = editing_tiles;
		m_tile_map.resize(int2(1024, 1024));
		m_entity_map.resize(int2(1024, 1024));

		loadTileGroup("data/tile_group.xml");
		m_group_editor = make_shared<GroupEditor>(IRect(m_left_width, 0, res.x, res.y));
		m_group_editor->setTarget(&m_group);

		m_mode_box =
			make_shared<ComboBox>(IRect(0, 0, m_left_width * 1 / 2, 22), 0, "Mode: ", s_mode_names);
		m_save_button =
			make_shared<Button>(IRect(m_left_width * 1 / 2, 0, m_left_width * 3 / 4, 22), "Save");
		m_load_button =
			make_shared<Button>(IRect(m_left_width * 3 / 4, 0, m_left_width, 22), "Load");

		m_group_pad =
			make_shared<GroupPad>(IRect(0, 22, m_left_width, res.y), m_group_editor, &m_group);

		m_left_window =
			make_shared<Window>(IRect(0, 0, m_left_width, res.y), WindowStyle::gui_dark);
		m_left_window->attach(m_mode_box);
		m_left_window->attach(m_save_button);
		m_left_window->attach(m_load_button);
		m_left_window->attach(m_group_pad);

		attach(m_left_window);
		attach(m_group_editor);

		//loadMap("data/maps/Assault/Lost Vault_mod.xml");
		loadMap("data/maps/mission02.mod");

		recreateEditors();
	}

	void freeEditors() {
		if(m_tiles_editor) {
			DASSERT(m_left_window);
			m_left_window->detach(m_tiles_pad);
			m_left_window->detach(m_entities_pad);
			detach(m_tiles_editor);
			detach(m_entities_editor);

			m_tiles_editor = nullptr;
			m_entities_editor = nullptr;
			m_tiles_pad = nullptr;
			m_entities_pad = nullptr;
		}
	}

	void recreateEditors() {
		freeEditors();

		IRect rect(m_left_width, 0, width(), height());
		m_view.emplace(m_tile_map, m_entity_map, rect.size());
		m_tiles_editor = make_shared<TilesEditor>(m_tile_map, *m_view.get(), rect);
		m_entities_editor =
			make_shared<EntitiesEditor>(m_tile_map, m_entity_map, *m_view.get(), rect);
		m_tiles_editor->setTileGroup(&m_group);

		m_tiles_pad =
			make_shared<TilesPad>(IRect(0, 22, m_left_width, height()), m_tiles_editor, &m_group);
		m_entities_pad =
			make_shared<EntitiesPad>(IRect(0, 22, m_left_width, height()), m_entities_editor);

		attach(m_tiles_editor);
		attach(m_entities_editor);
		m_left_window->attach(m_tiles_pad);
		m_left_window->attach(m_entities_pad);

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
		} else if(ev.type == Event::button_clicked && m_load_button.get() == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = make_shared<FileDialog>(dialog_rect, s_load_dialog_names[m_mode],
													FileDialogMode::opening_file);
			m_file_dialog->setPath("data/");
			attach(m_file_dialog, true);
		} else if(ev.type == Event::button_clicked && m_save_button.get() == ev.source) {
			IRect dialog_rect = IRect(-200, -150, 200, 150) + center();
			m_file_dialog = make_shared<FileDialog>(dialog_rect, s_save_dialog_names[m_mode],
													FileDialogMode::saving_file);
			m_file_dialog->setPath("data/");
			attach(m_file_dialog, true);
		} else if(ev.type == Event::window_closed && m_file_dialog.get() == ev.source) {
			if(ev.value && m_file_dialog->mode() == FileDialogMode::saving_file) {
				if(m_mode == editing_tiles || m_mode == editing_entities)
					saveMap(m_file_dialog->path().c_str());
				else
					saveTileGroup(m_file_dialog->path().c_str());
			} else if(ev.value && m_file_dialog->mode() == FileDialogMode::opening_file) {
				if(m_mode == editing_tiles || m_mode == editing_entities)
					loadMap(m_file_dialog->path().c_str());
				else
					loadTileGroup(m_file_dialog->path().c_str());
			}

			m_file_dialog = nullptr;
		} else
			return false;

		return true;
	}

	string mapName(FilePath path) const {
		if(path.isRelative())
			path = path.absolute().get();
		return FilePath(path).relative(m_maps_path);
	}

	void loadMap(Str file_name) {
		print("Loading Map: % ", file_name);
		fflush(stdout);
		double time = getTime();

		if(access(file_name)) {
			auto result = m_level.load(mapName(file_name));
			if(!result)
				result.error().print();
			recreateEditors();
		}
		printf("(%.2f sec)\n", getTime() - time);
	}

	void loadTileGroup(const char *file_name) {
		printf("Loading TileGroup: %s\n", file_name);
		if(access(file_name)) {
			auto doc = std::move(XmlDocument::load(file_name).get()); //TODO
			m_group.loadFromXML(doc);
		}
	}

	void saveMap(Str file_name) const {
		print("Saving Map: % ", file_name);
		fflush(stdout);
		double time = getTime();
		auto result = m_level.save(mapName(file_name));
		if(!result)
			result.error().print();
		printf(" (%.2f sec)\n", getTime() - time);
		//TODO: nie ma warninga ze nie udalo sie zapisac
	}

	void saveTileGroup(const char *file_name) const {
		printf("Saving TileGroup: %s\n", file_name);
		XmlDocument doc;
		m_group.saveToXML(doc);
		doc.save(file_name).check();
		//TODO: nie ma warninga ze nie udalo sie zapisac
	}

	bool mainLoop() {
		Tile::setFrameCounter((int)((getTime() - m_start_time) * 15.0));
		TextureCache::instance().nextFrame().check();

		process(m_gfx_device.window_ref->inputState());
		int2 window_size = m_gfx_device.window_ref->size();
		Canvas2D canvas(IRect(window_size), Orient2D::y_down);
		draw(canvas);
		m_gfx_device.drawFrame(canvas).check();
		TextureCache::instance().nextFrame().check();

		return true;
	}

	static bool mainLoop(VulkanWindow &window, void *pthis) {
		return ((EditorWindow *)pthis)->mainLoop();
	}

	GfxDevice &m_gfx_device;
	EditorMode m_mode;

	Level m_level;
	TileMap &m_tile_map;
	EntityMap &m_entity_map;
	TileGroup m_group;

	PComboBox m_mode_box;
	PButton m_save_button;
	PButton m_load_button;
	PFileDialog m_file_dialog;

	PWindow m_left_window;
	PGroupPad m_group_pad;
	PTilesPad m_tiles_pad;
	PEntitiesPad m_entities_pad;

	PView m_view;
	PTilesEditor m_tiles_editor;
	PEntitiesEditor m_entities_editor;
	PGroupEditor m_group_editor;

	int m_left_width;
	FilePath m_maps_path;
	double m_start_time = getTime();
};

void preloadTiles() {
	printf("Enumerating tiles\n");
	auto file_names = findFiles("data/tiles/", FindFileOpt::regular_file | FindFileOpt::recursive);

	printf("Preloading tiles");
	auto [prefix, suffix] = ResManager::instance().prefixSuffix(ResType::tile);
	auto current_path = FilePath::current().get(); // TODO
	FilePath tiles_path = FilePath(prefix).absolute(current_path);

	for(int n = 0; n < file_names.size(); n++) {
		ON_FAIL("\nError while loading file: %", file_names[n].path);

		if(n * 100 / file_names.size() > (n - 1) * 100 / file_names.size()) {
			printf(".");
			fflush(stdout);
		}

		FilePath tile_path = file_names[n].path.absolute(current_path).relative(tiles_path);
		string tile_name = tile_path;
		if(removeSuffix(tile_name, suffix))
			ResManager::instance().getTile(tile_name);
	}
	printf("\n");
}

Ex<int> exMain() {
	Config config("editor");

	auto gfx_device = EX_PASS(GfxDevice::create("editor", config));

	ResManager res_mgr(gfx_device.device_ref);
	TextureCache tex_cache(*gfx_device.device_ref);
	preloadTiles();
	game::loadData(true);

	EditorWindow window(gfx_device);
	gfx_device.window_ref->runMainLoop(&EditorWindow::mainLoop, &window);

	return 0;
}

int main(int argc, char **argv) {
	auto result = exMain();
	if(!result) {
		result.error().print();
		return 1;
	}
	return *result;
}
