/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "gfx/device.h"
#include "gfx/font.h"

#include "game/sprite.h"
#include "game/tile.h"

#include "ui/list_box.h"
#include "ui/button.h"
#include "ui/message_box.h"
#include "sys/platform.h"
#include "sys/config.h"

using namespace gfx;
using namespace game;
using namespace ui;
using namespace sys;

namespace ResType {
	enum Type {
		empty,
		tile,
		sprite,
		texture,
	};
};

class Resource {
	Resource(ResType::Type type, int id) :m_type(type), m_id(id) {
		m_font = gfx::Font::mgr[ui::WindowStyle::fonts[0]];
	}
public:
	Resource() :Resource(ResType::empty, -1) { }
	Resource(PTile res, int id) :Resource(ResType::tile, id) {
		DASSERT(res);
		m_resource = res.get();
		m_rect_size = res->rect().size() + int2(8, 8);
	}

	Resource(PTexture res, int id) :Resource(ResType::texture, id) {
		DASSERT(res);
		m_resource = res.get();
		m_rect_size = res->dimensions();
	}

	Resource(PSprite res, int id) :Resource(ResType::sprite, id) {
		DASSERT(res);
		m_resource = res.get();
		m_rect_size = worldToScreen(IBox({-4, -4, -4}, res->bboxSize() + int3{4,4,4})).size();

		m_last_time = getTime();
		m_frame_id = m_dir_id = m_seq_id = 0;
	}

	void printStats(int2 pos) const {
		char buffer[1024] = "";
		if(m_type == ResType::sprite) {
			const Sprite *sprite = static_cast<const Sprite*>(m_resource.get());
			bool is_gui_image = (*sprite)[m_seq_id].name.find("gui") != string::npos;
			auto &seq = (*sprite)[m_seq_id];

			int2 max_frame_size(0, 0);
			for(int n = 0; n < seq.frame_count; n++)
				max_frame_size = max(max_frame_size, sprite->getRect(m_seq_id, n, m_dir_id).size());

			snprintf(buffer, sizeof(buffer), "Sequence: %d / %d\n%s\nFrames: %d\nBounding box: (%d, %d, %d)\nMax frame size: (%d, %d)",
					m_seq_id, (int)sprite->size(), seq.name.c_str(), seq.frame_count,
					sprite->bboxSize().x, sprite->bboxSize().y, sprite->bboxSize().z,
					max_frame_size.x, max_frame_size.y);

			m_font->drawShadowed(pos, Color::white, Color::black, buffer);
			pos.y += m_font->evalExtents(buffer).height();

			double time = getTime();
			vector<pair<const char*, double>> tevents;
			for(int n = 0; n < (int)m_events.size(); n++)
				if(m_events[n].second > time - 1.0)
					tevents.push_back(m_events[n]);
			m_events.swap(tevents);

			for(int n = 0; n < (int)m_events.size(); n++) {
				Color col((float)(m_events[n].second - time + 1.0), 0.0f, 0.0f);
				m_font->drawShadowed(pos, col, Color::black, m_events[n].first);
				pos.y += m_font->textBase();
			}

		
		}
		else if(m_type == ResType::texture) {
			const DTexture *texture = static_cast<const DTexture*>(m_resource.get());
			snprintf(buffer, sizeof(buffer), "Size: (%d, %d)",
					texture->width(), texture->height());
			m_font->drawShadowed(pos, Color::white, Color::black, buffer);
		}
	}

	void draw(int2 pos, bool is_selected) const {
		Color outline_col = is_selected? Color::red : Color(255, 255, 255, 100);

		if(m_type == ResType::tile) {
			const Tile *tile = static_cast<const Tile*>(m_resource.get());

			lookAt(-pos + tile->rect().min);
			IBox box(int3(0, 0, 0), tile->bboxSize());

			//TODO: draw only if its visible, otherwise it might create some performance
			// problems if texture cache is full
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

			if(isKeyDown('E')) {
				string name = sys::Path(texture->resourceName()).relative();
				removeSuffix(name, ".zar");
				name += ".tga";
				printf("Exporting: %s\n", name.c_str());
				gfx::Texture tex;
				texture->download(tex);
				Saver svr(name);
				tex.save(svr);
			}
		}
		else if(m_type == ResType::sprite) {
			const Sprite *sprite = static_cast<const Sprite*>(m_resource.get());

			bool is_gui_image = (*sprite)[m_seq_id].name.find("gui") != string::npos;

			int id;
			while((id = sprite->frame(m_seq_id, m_frame_id).id) < 0) {
				const char *event_name = Sprite::eventIdToString((Sprite::EventId)id);
				if(id == Sprite::ev_overlay)
					event_name = "overlay";
				if(event_name)
					m_events.push_back(make_pair(event_name, getTime()));

				m_frame_id++;
				if(m_frame_id == sprite->frameCount(m_seq_id))
					m_frame_id = 0;
			}

			IRect rect = sprite->getRect(m_seq_id, m_frame_id, m_dir_id);
			FRect tex_rect;
			PTexture dtex = sprite->getFrame(m_seq_id, m_frame_id, m_dir_id, tex_rect);
			dtex->bind();

			IBox box({0,0,0}, sprite->bboxSize());
			IRect brect = worldToScreen(IBox(box.min - int3(4,4,4), box.max + int3(4,4,4)));
			if(is_gui_image) {
				rect -= rect.min;
				brect -= brect.min;
			}
			lookAt(brect.min - pos);
			drawQuad(rect.min, rect.size(), tex_rect.min, tex_rect.max);
	
			DTexture::bind0();
			if(is_gui_image)
				drawRect(rect, outline_col);
			else
				drawBBox(box, outline_col);

			double time = getTime();
			if(time - m_last_time > 1 / 15.0) {
				m_frame_id = m_frame_id + 1;
				m_last_time = time;
			}
			if(isKeyDown(Key_up))
				m_seq_id++;
			if(isKeyDown(Key_down))
				m_seq_id--;
			if(isKeyDown(Key_left))
				m_dir_id--;
			if(isKeyDown(Key_right))
				m_dir_id++;
			if(isKeyDown('P')) {
				Path path(sprite->resourceName());
				printf("Sequences for: %s\n", path.relative().c_str());
				for(int s = 0; s < sprite->size(); s++)
					printf("Seq %3d: %s\n", s, (*sprite)[s].name.c_str());
				printf("\n");
			}

			m_seq_id = (m_seq_id + (int)sprite->size()) % (int)sprite->size();
			int dir_count = sprite->dirCount(m_seq_id);
			m_dir_id = (m_dir_id + dir_count) % dir_count;
			m_frame_id %= sprite->frameCount(m_seq_id);
		}
	}

	ResType::Type type() const { return m_type; }
	int2 rectSize() const { return m_rect_size; }
	int id() const { return m_id; }

private:
	int2 m_rect_size;
	Ptr<RefCounter> m_resource;
	PFont m_font;
	ResType::Type m_type;
	int m_id;
	
	mutable vector<pair<const char *, double> > m_events;
	mutable double m_last_time;
	mutable int m_frame_id, m_seq_id, m_dir_id;
};

class ResourceView: public Window
{
public:
	virtual const char *className() const { return "ResourceView"; }
	ResourceView(IRect rect) :Window(rect), m_selected_id(-1), m_show_selected(false) { }

	void clear() {
		m_resources.clear();
		m_selected_id = -1;
	}

	void select(int new_id) {
		m_show_selected = new_id != m_selected_id;
		m_selected_id = new_id;
	}

	void drawContents() const {
		int spacing = 4;

		int2 pos(spacing, spacing + 100), offset = innerOffset() - clippedRect().min;
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
			cur_height = max(cur_height, res.rectSize().y);

			if(n + 1 == (int)m_resources.size())
				break;

			pos.x += res.rectSize().x + spacing;

			if(m_resources[n + 1].rectSize().x + pos.x > width) {
				pos.x = spacing;
				pos.y += cur_height + spacing;
				cur_height = 0;
			}
		}

		if(m_selected_id != -1) {
			lookAt(-clippedRect().min);
			for(int n = 0; n < (int)m_resources.size(); n++)
				if(m_resources[n].id() == m_selected_id) {
					m_resources[n].printStats(int2(0, 0));
					break;
				}
		}

		if(m_show_selected) {
			offset = int2(0, selected_pos.y);
			m_show_selected = false;
		}
		if(clicked)		
			mthis->sendEvent(mthis, Event::element_selected, m_selected_id);
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
			//	printf("Loading image: %s\n", file_name);
				Loader(file_name) >> *tex;
				res = ::Resource(tex, id);
				tex->setResourceName(file_name);
			}
			else if(strcasecmp(file_name + len - 5, ".tile") == 0) {
				PTile tile = new Tile;
			//	printf("Loading tile: %s\n", file_name);
				Loader(file_name) >> *tile;
				res = ::Resource(tile, id);
				tile->setResourceName(file_name);
			}
			else if(strcasecmp(file_name + len - 7, ".sprite") == 0) {
				PSprite sprite = new Sprite;
			//	printf("Loading sprite: %s\n", file_name);
				Loader(file_name) >> *sprite;
				res = ::Resource(sprite, id);
				sprite->printInfo();
				sprite->setResourceName(file_name);
			}
			else
				return;

			m_resources.push_back(res);
		}
		catch(const Exception &ex) { printf("%s\n", ex.what()); }
	}

private:
	mutable bool m_show_selected;
	int m_selected_id;
	vector< ::Resource> m_resources;
};

class ResViewerWindow: public Window
{
public:
	ResViewerWindow(int2 res) :Window(IRect(0, 0, res.x, res.y), WindowStyle::gui_light) {
		int left_width = 300;

		m_dir_view = new ListBox(IRect(0, 0, left_width, res.y));
		m_res_view = new ResourceView(IRect(left_width + 2, 0, res.x, res.y));

		attach(m_dir_view.get());
		attach(m_res_view.get());

		m_current_dir = "data/";
		m_current_dir = m_current_dir.absolute();

		update();
	}

	void update() {
		m_dir_view->clear();
		m_entries.clear();

		m_entries.clear();
		findFiles(m_entries, m_current_dir, FindFiles::regular_file | FindFiles::directory | FindFiles::relative);

		sort(m_entries.begin(), m_entries.end());
		for(int n = 0; n < (int)m_entries.size(); n++)
			m_dir_view->addEntry(m_entries[n].path.c_str(), m_entries[n].is_dir? Color::yellow : Color::white);
		
		m_res_view->clear();
		
		for(int n = 0; n < (int)m_entries.size(); n++)
			if(!m_entries[n].is_dir)
				m_res_view->tryAddResource((m_current_dir / m_entries[n].path).c_str(), n);
	}

	virtual bool onEvent(const Event &ev) {
		if(ev.type == Event::window_closed && ev.source == popup) {
			popup = nullptr;
			if(ev.value == 1)
				exit(0);
		}
		else if(ev.type == Event::element_selected) {
			if(m_dir_view.get() == ev.source && ev.value >= 0 && ev.value < (int)m_entries.size()) {
				const FileEntry &entry = m_entries[ev.value];

				if(entry.is_dir) {
					m_current_dir /= entry.path;
					update();
				}
				else {
					m_res_view->select(ev.value);
				}
			}
			else if(m_res_view.get() == ev.source) {
				m_dir_view->selectEntry(ev.value);
			}
		}
		else if(ev.type == Event::escape) {
			if(!popup) {
				IRect popup_rect = IRect(-150, -40, 150, 40) + center();
				popup = new MessageBox(popup_rect, "Do you want to quit?", MessageBoxMode::yes_no);
				attach(popup, true);
			}
		}
		else return false;

		return true;
	}

	vector<FileEntry> m_entries;
	sys::Path m_current_dir;

	PListBox			m_dir_view;
	Ptr<ResourceView>	m_res_view;

	PWindow popup;
};


int safe_main(int argc, char **argv)
{
	Config config = loadConfig("res_viewer");

	createWindow(config.resolution, config.fullscreen);
	setWindowTitle("FreeFT::res_viewer; built " __DATE__ " " __TIME__);
	grabMouse(false);

	setBlendingMode(bmNormal);

	ResViewerWindow main_window(config.resolution);
	clear(Color(0, 0, 0));

	double start_time = getTime();
	while(pollEvents()) {
		Tile::setFrameCounter((int)((getTime() - start_time) * 15.0));

		main_window.process();
		main_window.draw();

		swapBuffers();
		TextureCache::main_cache.nextFrame();
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

