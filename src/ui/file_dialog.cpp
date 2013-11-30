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

		PTextBox title_box = new TextBox(IRect(5, 5, w - 5, 25), title);
		title_box->setFont(s_font_names[1]);

		m_list_box = new ListBox(IRect(5, 25, w - 25, h - 50));
		m_edit_box = new EditBox(IRect(5, h - 49, w - 5, h - 26), 30, Color::gui_light);
		m_ok_button = new Button(IRect(w - 110, h - 25, w - 55, h - 5), s_ok_label[mode], 1);
		PButton cancel_button = new Button(IRect(w - 55 , h - 25, w - 5, h - 5), "cancel", 0);

		attach(title_box.get());
		attach(m_ok_button.get());
		attach(cancel_button.get());
		attach(m_list_box.get());
		attach(m_edit_box.get());
	
		m_dir_path = "./";
		updateList();
	}
	
	void FileDialog::setPath(const Path &path) {
		if(path.isDirectory()) {
			m_dir_path = path;
			m_edit_box->setText("");
		}
		else {
			m_dir_path = path.parent();
			m_edit_box->setText(path.fileName().c_str());
		}
		updateList();
		updateButtons();
	}

	const Path FileDialog::path() const {
		return m_dir_path / m_edit_box->text();
	}

	void FileDialog::drawContents() const {
		drawWindow(IRect({0, 0}, rect().size()), Color::gui_dark, 3);
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
				Path file_path = m_dir_path / Path((*m_list_box)[event.value].text.c_str());
				file_path = file_path.absolute();

				if(file_path.isDirectory()) {
					m_dir_path = file_path;
					updateList();
				}
				else if(file_path.isRegularFile()) {
					m_edit_box->setText(file_path.fileName().c_str());
				}
			}
			updateButtons();
		}
		else if(event.type == Event::text_modified) {
			const char *file = m_edit_box->text();
			m_list_box->selectEntry(m_list_box->findEntry(file));
			updateButtons();
		}
		else
			return false;

		return true;
	}

	void FileDialog::updateList() {
		m_list_box->clear();

		vector<FileEntry> files;
		findFiles(files, m_dir_path, FindFiles::directory | FindFiles::regular_file | FindFiles::relative);

		std::sort(files.begin(), files.end());
		for(int n = 0; n < (int)files.size(); n++)
			m_list_box->addEntry(files[n].path.c_str(), files[n].is_dir? Color::yellow : Color::white);
	}

	void FileDialog::updateButtons() {
		Path file_path = m_dir_path / Path(m_edit_box->text());

		if(m_mode == FileDialogMode::opening_file)
			m_ok_button->enable(file_path.isRegularFile());
		else if(m_mode == FileDialogMode::saving_file)
			m_ok_button->enable(!file_path.isDirectory());
	}

}
