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

#ifndef UI_MESSAGE_BOX_H
#define UI_MESSAGE_BOX_H


#include "ui/window.h"

namespace ui { 

	namespace MessageBoxMode {
		enum Type {
			yes_no,
			ok_cancel,
			ok,
		};
	}

	class MessageBox: public Window
	{
	public:
		typedef MessageBoxMode::Type Mode;
		MessageBox(const IRect &rect, const char *message, Mode mode);
		virtual const char *typeName() const { return "MessageBox"; }

		bool onEvent(const Event &event);
		void drawContents() const;

	private:
		Mode m_mode;
	};

}


#endif
