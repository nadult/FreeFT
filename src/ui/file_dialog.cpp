#include "ui/file_dialog.h"
#include "ui/text_box.h"

using namespace boost::filesystem;
typedef boost::filesystem::path Path;

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
	
	void FileDialog::setPath(const char *tpath) {
		Path path = tpath;
		if(is_directory(path)) {
			m_dir_path = path;
			m_edit_box->setText("");
		}
		else {
			m_dir_path = path.parent_path();
			m_edit_box->setText(path.filename().c_str());
		}
		updateList();
		updateButtons();
	}

	string FileDialog::path() const {
		return m_dir_path.string() + string(m_edit_box->text());
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
				Path file_path = m_dir_path;
				file_path /= Path((*m_list_box)[event.value].text.c_str());

				if(is_directory(file_path)) {
					m_dir_path = file_path;
					updateList();
				}
				else {
					m_edit_box->setText(file_path.filename().c_str());
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

        vector<Path> files, dirs;
        copy(directory_iterator(m_dir_path), directory_iterator(), back_inserter(files));
        sort(files.begin(), files.end());
		dirs.push_back("..");

		for(int n = 0; n < (int)files.size(); n++) {
			if(is_directory(files[n]))
				dirs.push_back(files[n]);
			if(!is_regular_file(files[n])) {
				files[n] = files.back();
				files.pop_back();
				n--;
			}
		}

		for(int n = 0; n < (int)dirs.size(); n++)
			m_list_box->addEntry(dirs[n].filename().c_str(), Color::yellow);
		for(int n = 0; n < (int)files.size(); n++)
			m_list_box->addEntry(files[n].filename().c_str(), Color::white);
	}

	void FileDialog::updateButtons() {
		Path file_path = m_dir_path;
		file_path /= Path(m_edit_box->text());

		if(m_mode == FileDialogMode::opening_file) {
			m_ok_button->enable(is_regular_file(file_path));
		}
		else if(m_mode == FileDialogMode::saving_file) {
			m_ok_button->enable(!is_directory(file_path) && !is_symlink(file_path) && !is_other(file_path));
		}
	}

}
