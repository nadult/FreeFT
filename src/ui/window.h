#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "base.h"
#include <memory>

namespace ui
{

	class Window;
	typedef std::unique_ptr<Window> PWindow;

	class Window
	{
	public:
		Window(IRect rect, Color background);
		virtual ~Window() { }
		
		virtual void handleInput();
		virtual void draw() const;

		void addChild(PWindow&&);

		void setRect(IRect rect);
		IRect rect() const { return m_rect; }

		void setBackgroundColor(Color col) { m_background_color = col; }
	
	protected:
		virtual void drawContents() const { }
		virtual void onInput(int2 mouse_pos) { }
		virtual void onIdle();

		// each on*** function should return true if the event was handled
		virtual bool onMouseClick(int2 pos, int key) { return false; }
		virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final) { return false; }

	private:
		void updateRects();

		Window *m_parent;
		vector<PWindow> m_children;

		IRect m_rect;			// coordinates relative to parent window
		IRect m_clipped_rect;	// global coordinates, clipped to parent window
		Color m_background_color;

		int2 m_drag_start;
		int m_dragging_mode;
		bool m_is_dragging;
	};

}

#endif

