#ifndef UI_MESSAGE_BOX_H
#define UI_MESSAGE_BOX_H


#include "ui/window.h"

namespace ui { 

	class MessageBox: public Window
	{
	public:
		MessageBox(const IRect &rect, const char *message);
		bool onEvent(const Event &event);
		void drawContents() const;
	};

}


#endif
