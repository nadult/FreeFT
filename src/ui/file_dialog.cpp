#include "ui/file_dialog.h"

using namespace boost::filesystem;
typedef boost::filesystem::path Path;

namespace ui {

	static const char *s_ok_label[FileDialogMode::count] = {
		"open",
		"save",
		"open",
	};

	FileDialog::FileDialog(const IRect &rect, const char *ext, Mode mode)
		:Window(rect, Color::transparent), m_ext(ext), m_mode(mode) {
		ASSERT(mode != FileDialogMode::opening_directory);

		int w = width(), h = height();
		m_list_box = new ListBox(IRect(5, 5, w - 5, h - 50));
		m_edit_box = new EditBox(IRect(5, h - 49, w - 5, h - 26), 30, Color::gui_light);

		attach(new Button(IRect(w - 110, h - 25, w - 55, h - 5), s_ok_label[mode], 1));
		attach(new Button(IRect(w - 55 , h - 25, w - 5, h - 5), "cancel", 0));
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
	}

	string FileDialog::path() const {
		return m_dir_path.string() + string(m_edit_box->text());
	}

	void FileDialog::drawContents() const {
		drawWindow(IRect({0, 0}, rect().size()), Color::gui_dark, 3);
	}

	bool FileDialog::onEvent(const Event &event) {
		if(event.type == Event::button_clicked) {
			close(event.value);
			return true;
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
		}
		else if(event.type == Event::text_modified) {
			const char *file = m_edit_box->text();
			m_list_box->selectEntry(m_list_box->findEntry(file));
		}

		return false;
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

}
