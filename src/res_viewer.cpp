#include <memory.h>
#include <cstdio>
#include <algorithm>
#include <unistd.h>

#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/sprite.h"
#include "gfx/tile.h"

#include "ui/list_view.h"
#include "ui/button.h"
#include "ui/message_box.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace gfx;

namespace ResType {
	enum Type {
		empty,
		tile,
		sprite,
		texture,
	};
};

class Resource {
public:
	Resource() :m_type(ResType::empty), m_id(-1) { }
	Resource(PTile res, int id) :m_type(ResType::tile), m_id(id) {
		DASSERT(res && res->dTexture);
		m_resource = res.get();
		m_rect_size = res->dTexture->size();
	}

	Resource(PTexture res, int id) :m_type(ResType::texture), m_id(id) {
		DASSERT(res);
		m_resource = res.get();
		m_rect_size = res->size();
	}

	Resource(PSprite res, int id) :m_type(ResType::sprite), m_id(id) {
		DASSERT(res);
		m_resource = res.get();
		m_rect_size = worldToScreen(IBox({0, 0, 0}, res->m_bbox)).size();
		for(int a = 0; a < (int)res->m_anims.size(); a++)
			for(int i = 0; i < (int)res->m_anims[a].images.size(); i++)
				m_rect_size = max(m_rect_size, res->m_anims[a].images[i].size);
	}

	void draw(int2 pos, bool is_selected) const {
		Color outline_col = is_selected? Color::red : Color::white;

		if(m_type == ResType::tile) {
			const Tile *tile = static_cast<const Tile*>(m_resource.get());

			lookAt(-pos - tile->m_offset);
			IBox box(int3(0, 0, 0), tile->m_bbox);
			tile->draw(int2(0, 0));

			DTexture::bind0();
			drawBBox(box, outline_col);
		}
		else if(m_type == ResType::texture) {
			const DTexture *texture = static_cast<const DTexture*>(m_resource.get());

			lookAt(-pos);
			texture->bind();
			drawQuad({0, 0}, m_rect_size);
			DTexture::bind0();
			drawRect(IRect({0, 0}, m_rect_size), outline_col);
		}
		else if(m_type == ResType::sprite) {
			const Sprite *sprite = static_cast<const Sprite*>(m_resource.get());
			Sprite::Rect srect;

			Texture tex = sprite->getFrame(0, 0, 0, &srect);
			DTexture dtex;
			dtex.setSurface(tex);
			dtex.bind();

			IRect rect = IRect(srect.left, srect.top, srect.right, srect.bottom) - sprite->m_offset;
			lookAt(-pos);
			drawQuad({0, 0}, rect.size());
		
			DTexture::bind0();	
			lookAt(-pos + rect.min);
			drawBBox(IBox({0,0,0}, sprite->m_bbox), outline_col);
		}
	}

	ResType::Type type() const { return m_type; }
	int2 rectSize() const { return m_rect_size; }
	int id() const { return m_id; }

private:
	int2 m_rect_size;
	Ptr<RefCounter> m_resource;
	ResType::Type m_type;
	int m_id;
};

class ResourceView: public ui::Window
{
public:
	virtual const char *className() const { return "ResourceView"; }
	ResourceView(IRect rect) :ui::Window(rect), m_selected_id(-1), m_show_selected(false) { }

	void clear() {
		m_resources.clear();
		m_selected_id = -1;
	}

	void drawContents() const {
		int spacing = 4;

		int2 pos(spacing, spacing), offset = innerOffset() - clippedRect().min;
		int width = clippedRect().width(), cur_height = 0;
		int2 mouse_pos = getMousePos();
		bool clicked = isMouseKeyPressed(0) && clippedRect().isInside(mouse_pos);
		int2 selected_pos(0, 0);
		
		//TODO: fix it
		ResourceView *mthis = (ResourceView*)this;
		if(clicked)
			mthis->m_selected_id = -1;

		for(int n = 0; n < (int)m_resources.size(); n++) {
			const ::Resource &res = m_resources[n];
			if(clicked && IRect(pos - offset, pos - offset + res.rectSize()).isInside(mouse_pos))
				mthis->m_selected_id = res.id();

			bool is_selected = m_selected_id == res.id();
			if(is_selected)
				selected_pos = pos;
			res.draw(pos - offset, is_selected);
			if(n + 1 == (int)m_resources.size())
				break;

			pos.x += res.rectSize().x + spacing;
			cur_height = max(cur_height, res.rectSize().y);

			if(m_resources[n + 1].rectSize().x + pos.x > width) {
				pos.x = spacing;
				pos.y += cur_height + spacing;
				cur_height = 0;
			}
		}
		
		if(m_show_selected) {
			offset = int2(0, selected_pos.y);
			m_show_selected = false;
		}
		if(clicked)		
			mthis->sendEvent(mthis, ui::Event::element_selected, m_selected_id);
		mthis->setInnerRect(IRect(-offset, int2(width, pos.y + cur_height) - offset));
	}

	void tryAddResource(const char *file_name, int id) {
		int len = (int)strlen(file_name);
		if(len < 4)
			return;

		try {
			::Resource res;
			if(strcasecmp(file_name + len - 4, ".zar") == 0 || strcasecmp(file_name + len - 4, ".png") == 0) {
				PTexture tex = new DTexture;
				printf("Loading image: %s\n", file_name);
				Loader(file_name) & *tex;
				res = ::Resource(tex, id);
			}
			else if(strcasecmp(file_name + len - 4, ".til") == 0) {
				PTile tile = new Tile;
				printf("Loading tile: %s\n", file_name);
				Loader(file_name) & *tile;
				tile->loadDTexture();
				res = ::Resource(tile, id);
			}
			else if(strcasecmp(file_name + len - 4, ".spr") == 0) {
				PSprite sprite = new Sprite;
				printf("Loading sprite: %s\n", file_name);
				Loader(file_name) & *sprite;
				res = ::Resource(sprite, id);
			}

			m_resources.push_back(res);
		}
		catch(const Exception &ex) { printf("%s\n", ex.what()); }
	}

	mutable bool m_show_selected;
	int m_selected_id;
	vector< ::Resource> m_resources;
};

class ResViewerWindow: public ui::Window
{
public:
	ResViewerWindow(int2 res) :ui::Window(IRect(0, 0, res.x, res.y), Color::gui_light) {
		int left_width = 300;

		m_dir_view = new ui::ListView(IRect(0, 0, left_width, res.y));
		m_res_view = new ResourceView(IRect(left_width + 2, 0, res.x, res.y));

		attach(m_dir_view.get());
		attach(m_res_view.get());

		m_current_dir.push_back("../refs/");

		update();
	}

	void update() {
		m_dir_view->clear();
		m_entries.clear();
		//TODO: użyć jakiejś biblioteki do poruszania się po katalogach, np boost

		string current_dir;
		for(int n = 0; n < (int)m_current_dir.size(); n++)
			current_dir = current_dir + m_current_dir[n];

		DIR *dp = opendir(current_dir.c_str());
		if(!dp)
			THROW("Error while opening directory %s: %s", current_dir.c_str(), strerror(errno));

		try {
			struct dirent *dirp;

			while ((dirp = readdir(dp))) {
				char full_name[1024];
				struct stat file_info;

				if(strcmp(dirp->d_name, ".") == 0)
					continue;
				snprintf(full_name, sizeof(full_name), "%s/%s", current_dir.c_str(), dirp->d_name);
				if(lstat(full_name, &file_info) < 0)
					continue; //TODO: handle error

				m_entries.push_back(Entry{dirp->d_name, S_ISDIR(file_info.st_mode)});
			}
		}
		catch(...) {
			closedir(dp);
			throw;
		}
		closedir(dp);

		sort(m_entries.begin(), m_entries.end());
		for(int n = 0; n < (int)m_entries.size(); n++)
			m_dir_view->addEntry(m_entries[n].name.c_str(), m_entries[n].is_dir? Color::yellow : Color::white);
		
		m_res_view->clear();
		
		for(int n = 0; n < (int)m_entries.size(); n++)
			if(!m_entries[n].is_dir)
				m_res_view->tryAddResource((current_dir + m_entries[n].name).c_str(), n);
	}

	struct Entry {
		string name;
		bool is_dir;

		bool operator<(const Entry &rhs) const {
			return is_dir == rhs.is_dir? name < rhs.name : is_dir > rhs.is_dir;
		}
	};

	virtual bool onEvent(const ui::Event &evt) {
		if(evt.type == ui::Event::window_closed && evt.source == popup) {
			popup = nullptr;
			if(evt.value == 1)
				exit(0);
		}
		else if(evt.type == ui::Event::element_selected) {
			if(m_dir_view == evt.source && evt.value >= 0 && evt.value < (int)m_entries.size()) {
				const Entry &entry = m_entries[evt.value];

				if(entry.is_dir) {
					if(entry.name == "..") {
						if(m_current_dir.size() > 1)
							m_current_dir.pop_back();
					}
					else
						m_current_dir.push_back(entry.name + "/");
					update();
				}
				else {
					m_res_view->m_selected_id = evt.value;
					m_res_view->m_show_selected = true;
				}
			}
			else if(m_res_view == evt.source) {
				m_dir_view->select(evt.value);
			}
		}
		else return false;

		return true;
	}

	void exitMessageBox() {
		if(popup)
			return;
		popup = new ui::MessageBox(IRect(0, 0, 300, 80) + rect().size() / 2 - int2(150, 40), "do you want to quit?");
		attach(popup, true);
	}

	vector<Entry> m_entries;
	vector<string> m_current_dir;

	ui::PListView		m_dir_view;
	Ptr<ResourceView>	m_res_view;

	ui::PWindow popup;
};


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

	ResViewerWindow main_window(res);
	clear({0, 0, 0});

	while(pollEvents()) {
		if(isKeyDown(Key_esc))
			main_window.exitMessageBox();

		main_window.process();
		main_window.draw();

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

