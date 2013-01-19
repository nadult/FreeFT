/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef UI_FILE_DIALOG_H
#define UI_FILE_DIALOG_H


#include "ui/window.h"
#include "ui/button.h"
#include "ui/list_box.h"
#include "ui/edit_box.h"
#include "sys/platform.h"

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

		void setPath(const Path &path);
		const Path path() const;

	private:
		void updateList();
		void updateButtons();

		PButton m_ok_button;
		PListBox m_list_box;
		PEditBox m_edit_box;

		Mode m_mode;
		Path m_dir_path;
	};

	typedef Ptr<FileDialog> PFileDialog;

}



#endif
