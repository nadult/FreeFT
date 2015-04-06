/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef UI_FILE_DIALOG_H
#define UI_FILE_DIALOG_H


#include "ui/window.h"
#include "ui/button.h"
#include "ui/list_box.h"
#include "ui/edit_box.h"

namespace ui {

	namespace FileDialogMode {
		enum Type {
			opening_file,
			saving_file,
		};
	};

	class FileDialog: public Window
	{
	public:
		typedef FileDialogMode::Type Mode;

		FileDialog(const IRect &rect, const char *title, Mode mode);
		virtual void drawContents() const;
		virtual bool onEvent(const Event &event);

		Mode mode() const { return m_mode; }

		void setPath(const FilePath &path);
		FilePath path() const;

	private:
		void updateList();
		void updateButtons();

		PButton m_ok_button;
		PListBox m_list_box;
		PEditBox m_edit_box;

		Mode m_mode;
		FilePath m_dir_path;
	};

}



#endif
