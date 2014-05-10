/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/console.h"
#include "ui/edit_box.h"

using namespace gfx;
using namespace ui;

namespace io {

	class ConsoleEditBox: public ui::EditBox {
	public:
		ConsoleEditBox(const IRect &rect, int max_size, Color col)
			:EditBox(rect, max_size, "", col), m_new_command(false) { }

		void onInput(int2 mouse_pos) override {
			m_new_command = false;
			EditBox::onInput(mouse_pos);
		}

		bool onEvent(const Event &event) override {
			if(event.source == this && event.type == Event::text_modified) {
				m_new_command = true;
			}
			return EditBox::onEvent(event);
		}

		void reset() {
			m_new_command = false;
			EditBox::reset(true);
		}

		bool hasNewCommand() const { return m_new_command; }

	protected:
		bool m_new_command;
	};

	Console::Console(const int2 &resolution) :m_is_opened(false) {
		m_font = Font::mgr[WindowStyle::fonts[0]];
		m_edit_box = new ConsoleEditBox(IRect(0, 0, resolution.x, m_font->lineHeight() + 5), 128, Color(80, 140, 80, 200));
	}

	Console::~Console() { }

	void Console::open() {
		m_is_opened = true;
		m_edit_box->reset();
	}

	void Console::close() {
		m_is_opened = false;
	}
		
	const int2 Console::size() const {
		return m_is_opened? m_edit_box->rect().size() : int2(0, 0);
	}

	void Console::update(double time_diff) {
		if(isKeyDown('`')) {
			if(m_is_opened)
				close();
			else
				open();
			return;
		}
		if(isKeyPressed('`'))
			return;

		if(!m_is_opened)
			return;

		m_edit_box->setFocus(true);
		m_edit_box->process();

		if(m_edit_box->hasNewCommand()) {
			m_commands.push_back(m_edit_box->text());
			m_edit_box->reset();
		}
	}

	void Console::draw() const {
		if(!m_is_opened)
			return;

		m_edit_box->draw();
	}

	const string Console::getCommand() {
		string out;
		if(!m_commands.empty()) {
			out = m_commands.front();
			m_commands.erase(m_commands.begin());
		}
		return out;
	}

}
