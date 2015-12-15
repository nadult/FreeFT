/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/file_dialog.h"
#include "ui/text_box.h"
#include <algorithm>

namespace ui {

	static const char *s_ok_label[] = {
		"open",
		"save",
	};

	FileDialog::FileDialog(const IRect &rect, const char *title, Mode mode)
		:Window(rect, Color::transparent), m_mode(mode) {
		int w = width(), h = height();

		PTextBox title_box = make_shared<TextBox>(IRect(5, 5, w - 5, 25), title);
		title_box->setFont(WindowStyle::fonts[1]);

		m_list_box = make_shared<ListBox>(IRect(5, 25, w - 25, h - 50));
		m_edit_box = make_shared<EditBox>(IRect(5, h - 49, w - 5, h - 26), 30, "", WindowStyle::gui_light);
		m_ok_button = make_shared<Button>(IRect(w - 110, h - 25, w - 55, h - 5), s_ok_label[mode], 1);
		PButton cancel_button = make_shared<Button>(IRect(w - 55 , h - 25, w - 5, h - 5), "cancel", 0);

		attach(title_box);
		attach(m_ok_button);
		attach(cancel_button);
		attach(m_list_box);
		attach(m_edit_box);
	
		m_dir_path = "./";
		updateList();
	}
	
	void FileDialog::setPath(const FilePath &path) {
		if(path.isDirectory()) {
			m_dir_path = path;
			m_edit_box->setText(L"");
		}
		else {
			m_dir_path = path.parent();
			m_edit_box->setText(toWideString(path.fileName()));
		}
		updateList();
		updateButtons();
	}

	FilePath FileDialog::path() const {
		return m_dir_path / fromWideString(m_edit_box->text());
	}

	void FileDialog::drawContents(Renderer2D &out) const {
		drawWindow(out, IRect({0, 0}, rect().size()), WindowStyle::gui_dark, 3);
	}

	bool FileDialog::onEvent(const Event &event) {
		if(event.type == Event::escape) {
			close(0);
		}
		else if(event.type == Event::button_clicked) {
			close(event.value);
		}
		else if(event.type == Event::element_selected) {
			if(event.value >= 0 && event.value < m_list_box->size()) {
				FilePath file_path = m_dir_path / FilePath((*m_list_box)[event.value].text.c_str());
				file_path = file_path.absolute();

				if(file_path.isDirectory()) {
					m_dir_path = file_path;
					updateList();
				}
				else if(file_path.isRegularFile()) {
					m_edit_box->setText(toWideString(file_path.fileName()));
				}
			}
			updateButtons();
		}
		else if(event.type == Event::text_modified) {
			auto file_name  = fromWideString(m_edit_box->text());
			m_list_box->selectEntry(m_list_box->findEntry(file_name.c_str()));
			updateButtons();
		}
		else
			return false;

		return true;
	}

	void FileDialog::updateList() {
		m_list_box->clear();

		auto files = findFiles(m_dir_path, FindFiles::directory | FindFiles::regular_file | FindFiles::relative);

		std::sort(files.begin(), files.end());
		for(int n = 0; n < (int)files.size(); n++)
			m_list_box->addEntry(files[n].path.c_str(), files[n].is_dir? Color::yellow : Color::white);
	}

	void FileDialog::updateButtons() {
		FilePath file_path = m_dir_path / FilePath(fromWideString(m_edit_box->text()));

		if(m_mode == FileDialogMode::opening_file)
			m_ok_button->enable(file_path.isRegularFile());
		else if(m_mode == FileDialogMode::saving_file)
			m_ok_button->enable(!file_path.isDirectory());
	}

}
