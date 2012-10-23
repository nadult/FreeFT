#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui/window.h"
#include "gfx/font.h"

namespace ui
{

	class Button: public Window
	{
	public:
		Button(IRect rect, const char *text);


		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual void onIdle();

	protected:
		string m_text;
		gfx::PFont m_font;
		gfx::PTexture m_font_texture;
	};


}


#endif
