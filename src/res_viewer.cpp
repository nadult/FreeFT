// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "gfx/drawing.h"

#include "game/sprite.h"
#include "game/tile.h"
#include "res_manager.h"
#include "sys/config.h"
#include "sys/gfx_device.h"
#include "ui/button.h"
#include "ui/list_box.h"
#include "ui/message_box.h"

#include <fwk/gfx/canvas_2d.h>
#include <fwk/gfx/font.h>
#include <fwk/gfx/image.h>
#include <fwk/gfx/shader_compiler.h>
#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>
#include <fwk/str.h>
#include <fwk/sys/input.h>
#include <fwk/sys/on_fail.h>
#include <fwk/vulkan/vulkan_command_queue.h>
#include <fwk/vulkan/vulkan_device.h>
#include <fwk/vulkan/vulkan_image.h>
#include <fwk/vulkan/vulkan_instance.h>
#include <fwk/vulkan/vulkan_swap_chain.h>
#include <fwk/vulkan/vulkan_window.h>

#include <fwk/libs_msvc.h>

using namespace game;
using namespace ui;

class Resource {
  public:
	Resource() = default;
	Ex<void> load(VDeviceRef device, const FilePath &current_dir, const string &file_name) {
		m_file_name = file_name;
		m_type = ResManager::instance().classifyPath(file_name, true);
		EXPECT(m_type);

		auto path = current_dir / file_name;
		if(m_type == ResType::tile) {
			auto loader = EX_PASS(fileLoader(path));
			m_tile.emplace();
			EXPECT(m_tile->load(loader));
			m_rect_size = m_tile->rect().size() + int2(8, 8);
		} else if(m_type == ResType::texture) {
			auto image = EX_PASS(Image::load(path));
			m_rect_size = image.size();
			auto vimage = EX_PASS(VulkanImage::createAndUpload(device, {std::move(image)}));
			m_texture = VulkanImageView::create(device, vimage);
		} else if(m_type == ResType::sprite) {
			auto loader = EX_PASS(fileLoader(path));
			//	printf("Loading sprite: %s\n", file_name);
			m_sprite.emplace();
			EXPECT(m_sprite->load(loader));
			m_sprite->setResourceName(file_name);
			m_sprite->printInfo();
			m_rect_size =
				worldToScreen(IBox({-4, -4, -4}, m_sprite->bboxSize() + int3{4, 4, 4})).size();
			m_last_time = getTime();
			m_frame_id = m_dir_id = m_seq_id = 0;
			updateFrameId();
		}

		return {};
	}

	void printStats(Canvas2D &out, int2 pos, const Font &font) const {
		TextFormatter fmt;

		if(m_type == ResType::sprite) {
			bool is_gui_image = (*m_sprite)[m_seq_id].name.find("gui") != string::npos;
			auto &seq = (*m_sprite)[m_seq_id];

			int2 max_frame_size(0, 0);
			for(int n = 0; n < seq.frame_count; n++) {
				if(m_sprite->frame(m_seq_id, n).id < 0)
					continue;
				max_frame_size =
					max(max_frame_size, m_sprite->getRect(m_seq_id, n, m_dir_id).size());
			}

			fmt("Sequence: % / %\n%\nFrames: %\nBounding box: "
				"(%)\nMax frame size: (%)",
				m_seq_id, (int)m_sprite->size(), seq.name.c_str(), seq.frame_count,
				m_sprite->bboxSize(), max_frame_size);

			font.draw(out, (float2)pos, {ColorId::white, ColorId::black}, fmt.text());
			pos.y += font.evalExtents(fmt.text()).height();

			double time = getTime();
			for(int n = 0; n < (int)m_events.size(); n++) {
				FColor col((float)(m_events[n].second - time + 1.0), 0.0f, 0.0f);
				font.draw(out, (float2)pos, {IColor(col), ColorId::black}, m_events[n].first);
				pos.y += font.lineHeight();
			}

		} else if(m_type == ResType::texture) {
			fmt("Size: (%)", m_texture->size());
			font.draw(out, (float2)pos, {ColorId::white, ColorId::black}, fmt.text());
		}
	}

	void updateFrameId() {
		if(m_type != ResType::sprite)
			return;

		int id;
		while((id = m_sprite->frame(m_seq_id, m_frame_id).id) < 0) {
			const char *event_name = Sprite::eventIdToString((Sprite::EventId)id);
			if(id == int(Sprite::ev_overlay))
				event_name = "overlay";
			if(event_name)
				m_events.emplace_back(event_name, getTime());

			m_frame_id++;
			if(m_frame_id == m_sprite->frameCount(m_seq_id))
				m_frame_id = 0;
		}
	}

	void update(double time) {
		if(m_type == ResType::sprite) {
			if(time - m_last_time > 1 / 15.0) {
				m_frame_id = m_frame_id + 1;
				m_frame_id %= m_sprite->frameCount(m_seq_id);
				m_last_time = time;
			}

			updateFrameId();

			vector<pair<const char *, double>> tevents;
			for(int n = 0; n < (int)m_events.size(); n++)
				if(m_events[n].second > time - 1.0)
					tevents.push_back(m_events[n]);
			m_events.swap(tevents);
		}
	}

	void onInput(const InputState &state) {
		if(m_type == ResType::texture) {
			if(state.isKeyDown('E')) {
				string name = FilePath(m_file_name).relativeToCurrent().get();
				FATAL("fixme");
				/*removeSuffix(name, ".zar");
				name += ".tga";
				printf("Exporting: %s\n", name.c_str());
				Image tex;
				m_texture->download(tex);
				tex.saveTGA(name).check();*/
			}
		}
		if(m_type == ResType::sprite) {
			if(state.isKeyDown(InputKey::up))
				m_seq_id++;
			if(state.isKeyDown(InputKey::down))
				m_seq_id--;
			if(state.isKeyDown(InputKey::left))
				m_dir_id--;
			if(state.isKeyDown(InputKey::right))
				m_dir_id++;
			if(state.isKeyDown('P')) {
				FilePath path(m_file_name);
				print("Sequences for: %\n", path.relativeToCurrent().get());
				for(int s = 0; s < m_sprite->size(); s++)
					printf("Seq %3d: %s\n", s, (*m_sprite)[s].name.c_str());
				printf("\n");
			}

			m_seq_id = (m_seq_id + (int)m_sprite->size()) % (int)m_sprite->size();
			int dir_count = m_sprite->dirCount(m_seq_id);
			m_dir_id = (m_dir_id + dir_count) % dir_count;
			m_frame_id %= m_sprite->frameCount(m_seq_id);
			updateFrameId();
		}
	}

	void draw(Canvas2D &out, int2 pos, bool is_selected) const {
		Color outline_col = is_selected ? ColorId::red : Color(255, 255, 255, 100);

		if(m_type == ResType::tile) {
			out.setViewPos(-pos + m_tile->rect().min());
			IBox box(int3(0, 0, 0), m_tile->bboxSize());
			m_tile->draw(out, int2(0, 0));
			out.setMaterial({});
			drawBBox(out, box, outline_col);
		} else if(m_type == ResType::texture) {
			out.setViewPos(-pos);
			out.setMaterial(m_texture);
			out.addFilledRect(IRect(m_rect_size));
			out.setMaterial({});
			out.addRect(IRect({0, 0}, m_rect_size), outline_col);
		} else if(m_type == ResType::sprite) {
			bool is_gui_image = (*m_sprite)[m_seq_id].name.find("gui") != string::npos;

			DASSERT(m_sprite->frame(m_seq_id, m_frame_id).id >= 0);
			IRect rect = m_sprite->getRect(m_seq_id, m_frame_id, m_dir_id);
			FRect tex_rect;
			auto dtex = m_sprite->getFrame(m_seq_id, m_frame_id, m_dir_id, tex_rect);

			IBox box({0, 0, 0}, m_sprite->bboxSize());
			IRect brect = worldToScreen(IBox(box.min() - int3(4, 4, 4), box.max() + int3(4, 4, 4)));
			if(is_gui_image) {
				rect -= rect.min();
				brect -= brect.min();
			}
			out.setViewPos(brect.min() - pos);
			out.setMaterial({dtex, ColorId::white, none, SimpleBlendingMode::normal});
			out.addFilledRect(FRect(rect), tex_rect);
			out.setMaterial({});

			if(is_gui_image)
				out.addRect(rect, outline_col);
			else
				drawBBox(out, box, outline_col);
		}
	}

	auto type() const { return m_type; }
	int2 rectSize() const { return m_rect_size; }
	const string &fileName() const { return m_file_name; }

  private:
	string m_file_name;
	Maybe<ResType> m_type;

	int2 m_rect_size;
	Dynamic<Tile> m_tile;
	PVImageView m_texture;
	Dynamic<Sprite> m_sprite;

	vector<pair<const char *, double>> m_events;
	double m_last_time;
	int m_frame_id, m_seq_id, m_dir_id;
};

class ResourceView : public Window {
  public:
	virtual const char *className() const { return "ResourceView"; }
	ResourceView(VDeviceRef vdevice, IRect rect, FilePath current_dir, vector<string> file_names)
		: Window(rect), m_selected_id(-1), m_font(res::getFont(ui::WindowStyle::fonts[0])) {
		for(auto file_name : file_names) {
			if(ResManager::instance().classifyPath(file_name, true)) {
				ON_FAIL("Error while loading file: %", file_name);

				Resource new_resource;
				if(auto result = new_resource.load(vdevice, current_dir, file_name))
					m_resources.emplace_back(std::move(new_resource));
				else
					result.error().print();
			}
		}

		int spacing = 4, width = clippedRect().width(), cur_height = 0;
		int2 pos(spacing, spacing + 100);

		for(int n = 0; n < (int)m_resources.size(); n++) {
			auto &res = m_resources[n];
			m_positions.emplace_back(pos);
			cur_height = max(cur_height, res->rectSize().y);

			if(n + 1 < (int)m_resources.size()) {
				pos.x += res->rectSize().x + spacing;
				if(m_resources[n + 1]->rectSize().x + pos.x > width) {
					pos.x = spacing;
					pos.y += cur_height + spacing;
					cur_height = 0;
				}
			}
		}

		setInnerRect(IRect(int2(width, pos.y + cur_height)));
	}

	void select(const string &file_name) {
		for(int n = 0; n < (int)m_resources.size(); n++)
			if(m_resources[n]->fileName() == file_name) {
				if(m_selected_id != n)
					setInnerOffset(int2(0, m_positions[n].y - 5));
				m_selected_id = n;
			}
	}

	void update() {
		double time = getTime();
		for(auto &resource : m_resources)
			resource->update(time);
	}

	void onInput(const InputState &state) override {
		m_last_mouse_pos = state.mousePos();
		int2 offset = innerOffset() - clippedRect().min();

		for(auto &resource : m_resources)
			resource->onInput(state);

		bool clicked = state.isMouseButtonPressed(InputButton::left);

		if(clicked)
			m_selected_id = -1;
		for(int n = 0; n < (int)m_resources.size(); n++) {
			const auto &res = m_resources[n];
			int2 pos = m_positions[n] - offset;
			if(clicked && IRect(pos, pos + res->rectSize()).containsCell(m_last_mouse_pos))
				m_selected_id = n;
		}
		if(clicked)
			sendEvent(this, Event::element_selected, m_selected_id);
	}

	void drawContents(Canvas2D &out) const override {
		int2 offset = innerOffset() - clippedRect().min();

		for(int n = 0; n < (int)m_resources.size(); n++) {
			const auto &res = m_resources[n];

			auto pos = m_positions[n] - offset;
			if(pos.y + res->rectSize().y < clippedRect().y())
				continue;
			if(pos.y > clippedRect().ey())
				break;

			res->draw(out, m_positions[n] - offset, n == m_selected_id);
		}

		if(m_selected_id != -1) {
			out.setViewPos(-clippedRect().min());
			m_resources[m_selected_id]->printStats(out, int2(0, 0), m_font);
		}
	}

	const auto &operator[](int res_id) const {
		DASSERT(res_id >= 0 && res_id < (int)m_resources.size());
		return *m_resources[res_id];
	}

  private:
	int m_selected_id;
	int2 m_last_mouse_pos;
	vector<Dynamic<Resource>> m_resources;
	vector<int2> m_positions;
	const Font &m_font;
};

enum class Command { empty, change_dir, exit };

class ResViewerWindow : public Window {
  public:
	static constexpr int left_width = 300;

	ResViewerWindow(GfxDevice &gfx, int2 res, const string &path)
		: Window(IRect(res), WindowStyle::gui_light), m_gfx(gfx) {
		m_dir_view = make_shared<ListBox>(IRect(0, 0, left_width, res.y));
		attach(m_dir_view);
		setDir(path);
	}

	void setDir(const string &path) {
		m_current_dir = FilePath(path).absolute().get();
		// TODO: doesn't support links?
		m_entries = findFiles(m_current_dir, FindFileOpt::regular_file | FindFileOpt::directory |
												 FindFileOpt::link | FindFileOpt::relative |
												 FindFileOpt::include_parent);
		makeSorted(m_entries);
		vector<string> names;

		m_dir_view->clear();
		for(auto entry : m_entries) {
			auto color = entry.is_dir || entry.is_link ? ColorId::yellow : ColorId::white;
			m_dir_view->addEntry(entry.path.c_str(), color);
			if(!entry.is_dir && !entry.is_link)
				names.emplace_back(entry.path);
		}
		if(m_res_view)
			detach(m_res_view);
		m_res_view.reset(new ResourceView(m_gfx.device_ref, IRect({left_width + 2, 0}, size()),
										  m_current_dir, names));
		attach(m_res_view);
		m_command = {Command::empty, {}};
	}

	void resize(const int2 &new_size) {
		setRect(IRect(new_size));
		// TODO: resize controls
	}

	pair<Command, string> command() const { return m_command; }
	string currentDir() const { return m_current_dir; }

	void process(const InputState &state) override {
		m_res_view->update();
		Window::process(state);
	}

	bool onEvent(const Event &ev) override {
		if(ev.type == Event::element_selected) {
			if(m_dir_view.get() == ev.source && ev.value >= 0 && ev.value < (int)m_entries.size()) {
				const FileEntry &entry = m_entries[ev.value];

				if(entry.is_dir || entry.is_link)
					m_command = {Command::change_dir, m_current_dir / entry.path};
				else
					m_res_view->select((*m_dir_view)[ev.value].text);
			} else if(m_res_view.get() == ev.source) {
				int id = ev.value;
				if(id != -1) {
					auto &res = (*m_res_view)[id];
					for(int n = 0; n < m_dir_view->size(); n++)
						if((*m_dir_view)[n].text == res.fileName())
							id = n;
				}
				m_dir_view->selectEntry(id);
			}
		} else if(ev.type == Event::escape) {
			exit(0);
		} else
			return false;

		return true;
	}

	bool main_loop(VulkanWindow &vwindow) {
		Tile::setFrameCounter((int)((getTime() - m_start_time) * 15.0));
		TextureCache::instance().nextFrame().check();

		process(vwindow.inputState());
		auto command = this->command();
		if(command.first == Command::exit)
			return false;
		if(command.first == Command::change_dir)
			setDir(command.second);
		if(size() != vwindow.size())
			resize(vwindow.size());

		Canvas2D canvas(IRect(vwindow.size()), Orient2D::y_up);
		draw(canvas);
		m_gfx.drawFrame(canvas).check();

		return true;
	}

	static bool main_loop(VulkanWindow &vulkan_window, void *viewer_window) {
		return ((ResViewerWindow *)viewer_window)->main_loop(vulkan_window);
	}

  private:
	GfxDevice &m_gfx;
	vector<FileEntry> m_entries;
	FilePath m_current_dir;
	pair<Command, string> m_command = {Command::empty, string()};

	PListBox m_dir_view;
	shared_ptr<ResourceView> m_res_view;
	double m_start_time;
};

Ex<int> exMain() {
	Config config("res_viewer");
	auto gfx = EX_PASS(GfxDevice::create("res_viewer", config));
	ResManager res_mgr(gfx.device_ref);
	TextureCache tex_cache(*gfx.device_ref);

	ResViewerWindow res_viewer_window(gfx, gfx.window_ref->size(), "data/");
	gfx.window_ref->runMainLoop(&ResViewerWindow::main_loop, &res_viewer_window);
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
