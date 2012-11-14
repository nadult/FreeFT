#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "base.h"
#include <memory>

namespace gfx { class Tile; }

namespace ui
{

	class Button;
	class ListView;
	class Window;
	class MainWindow;

	typedef Ptr<Window> PWindow;

	class Window: public RefCounter
	{
	public:
		Window(IRect rect, Color background = Color::transparent);
		virtual ~Window() { }
		
		virtual void handleInput();
		virtual void draw() const;

		void addChild(PWindow&&);

		// it will also reset inner rect
		void setRect(const IRect &rect);
		IRect rect() const { return m_rect; }
		IRect clippedRect() const { return m_clipped_rect; }

		void setBackgroundColor(Color col);
		Window* parent() const { return m_parent; }
		MainWindow *mainWindow() { return m_parent? m_parent->mainWindow() : nullptr; }

		void setVisible(bool is_visible) { m_is_visible = is_visible; }
		bool isVisible() const { return m_is_visible; }

		bool isMouseOver() const;
		
		virtual void onButtonPressed(Button *button);
		virtual void onListElementClicked(ListView *list_view, int id);
		virtual void onEvent(Window *source, int event, int value);
		
		// popups are handled in MainWindow
		virtual void addPopup(PWindow&&);
		virtual void onClosePopup(Window *popup, int ret);

		static void drawWindow(IRect rect, Color color, int outline);
	
	protected:
		virtual void drawContents() const { }
		virtual void onInput(int2 mouse_pos) { }
		virtual void onIdle();

		// each on*** function should return true if the event was handled
		// TODO: pass key_modifier along with key (so when user presses LMB with CTRL, it will
		// be passed until LMB is released)
		virtual bool onMouseClick(int2 pos, int key, bool up) { return false; }
		virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final) { return false; }
		virtual bool onEscape() { return false; }

		// relative to normal rect
		// IRect(0, 0, rect.width(), rect.height()) is default
		void setInnerRect(const IRect &rect);
		IRect innerRect() const { return m_inner_rect; }

		int2 innerOffset() const { return -m_inner_rect.min; }
		void setInnerOffset(const int2&);

	private:
		friend class MainWindow;
		void updateRects();

		Window *m_parent;
		vector<PWindow> m_children;

		IRect  m_inner_rect;	// if its bigger than m_rect then progress bars will be added
		IRect m_rect;			// coordinates relative to parent window
		IRect m_clipped_rect;	// global coordinates, clipped to parent window
		Color m_background_color;

		int2 m_drag_start;
		int m_dragging_mode;
		bool m_is_dragging;
		bool m_is_visible;
		bool m_is_mouse_over;
		bool m_has_inner_rect;
	};

	class MainWindow: public Window
	{
	public:
		MainWindow(const IRect &rect, Color color) :Window(rect, color) { }
		MainWindow *mainWindow() { return this; }

		virtual void handleInput();
		virtual void draw() const;

		virtual void addPopup(PWindow&&);
		virtual void onClosePopup(Window *popup, int ret);

		bool anyPopups() const { return !m_popups.empty(); }

	private:
		vector<PWindow> m_popups;
	};

}

#endif

