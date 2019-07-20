// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "base.h"
#include <memory>
#include <fwk_input.h>
#include <fwk/gfx/renderer2d.h>
#include <fwk/gfx/font.h>

namespace ui
{

	struct Event {
		enum Type {
			window_closed,		// value: return value		sending down
			button_clicked,		// value: button id			sending up
			element_clicked,	// value: element id		sending up
			element_selected,	// value: element id		sending up
			popup_close_signal,	// 							sending directly to popup from parent
			escape,				//							sending down only to focused windows
			text_modified,		//							sending up
			progress_bar_moved,	//							sending up
		};

		Event(Window *source, Type type, int value = 0) :source(source), type(type), value(value) { }

		Window *source;
		Type type;
		int value;
	};

	//TODO: windows could be customizable with different styles
	struct WindowStyle {
		static FColor gui_dark;
		static FColor gui_medium;
		static FColor gui_light;
		static FColor gui_popup;
		
		enum {
			line_height = 24
		};

		// Default font names: small, normal, big
		static const char *fonts[3];
	};

	class Window
	{
	public:
		Window(const IRect &rect, FColor background = ColorId::transparent);

		virtual ~Window() { }
		virtual const char *typeName() const { return "Window"; }
		Window(const Window&) = delete;
		void operator=(const Window&) = delete;
		
		virtual void process(const InputState&);
		virtual void draw(Renderer2D&) const;

		void close(int return_value);

		// normally window is focused when the mouse is over, but it can
		// be overriden by calling this function
		// Popups have higher priority though
		void setFocus(bool set);
		void attach(PWindow, bool as_popup = false);
		void detach(PWindow);

		// it will also reset inner rect
		void setRect(const IRect &rect);
		const IRect localRect() const { return IRect(int2(0, 0), m_rect.size()); }
		const IRect &rect() const { return m_rect; }
		const IRect &clippedRect() const { return m_clipped_rect; }
		int width() const { return m_rect.width(); }
		int height() const { return m_rect.height(); }
		int2 size() const { return m_rect.size(); }
		int2 center() const { return m_rect.center(); }

		FColor backgroundColor() const { return m_background_color; }
		void setBackgroundColor(FColor);

		void setBackground(PTexture);
		PTexture background() const { return m_background; }

		Window *parent() const { return m_parent; }
		Window *mainWindow() { return m_parent? m_parent->mainWindow() : this; }

		void setVisible(bool is_visible) { m_is_visible = is_visible; }
		bool isVisible() const { return m_is_visible; }
		bool isFocused() const;
		bool isMouseOver() const;
		bool isPopup() const { return m_is_popup; }

		//TODO: should events be sent to unfocused objects?

		// Sends event up (towards main window) or down the hierarchy
		// returns true if event was received
		bool sendEvent(const Event &event);
		bool sendEvent(Window *source, Event::Type type, int value = 0) {
			return sendEvent(Event(source, type, value));
		}

		// Override this method to receive events
		virtual bool onEvent(const Event &event) { return false; }
		
		static void drawWindow(Renderer2D&, IRect, FColor, int outline);

		void setInnerOffset(const int2&);
	
	protected:
		virtual void drawContents(Renderer2D&) const { }
		virtual void onInput(const InputState&) { }

		// each on*** function should return true if the event was handled
		// TODO: pass key_modifier along with key (so when user presses LMB with CTRL, it will
		// be passed until LMB is released)
		virtual bool onMouseClick(const InputState&, int2 pos, int key, bool up) { return false; }

		// is_final == -1: dragging has been cancelled (with esc-key)
		virtual bool onMouseDrag(const InputState&, int2 start, int2 current, int key, int is_final) { return false; }

		// relative to normal rect
		// IRect(0, 0, rect.width(), rect.height()) is default
		void setInnerRect(const IRect &rect);
		IRect innerRect() const { return m_inner_rect; }

		int2 innerOffset() const { return -m_inner_rect.min(); }

	private:
		void updateRects();

		Window *m_parent;
		vector<PWindow> m_children;
		PTexture m_background;

		IRect  m_inner_rect;	// if its bigger than m_rect then progress bars will be added
		IRect m_rect;			// coordinates relative to parent window
		IRect m_clipped_rect;	// absolute coordinates, clipped to parent window
		FColor m_background_color;

		int2 m_drag_start;
		int m_closing_value;
		int m_dragging_mode : 4;
		bool m_is_visible : 1;
		bool m_has_inner_rect : 1;
		bool m_is_popup : 1;
		bool m_is_closing : 1;
		bool m_is_focused : 1;
		bool m_has_hard_focus : 1;
		bool m_is_mouse_over: 1;
	};

}

#endif

