/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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
