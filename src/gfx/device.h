/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include "base.h"
#include "gfx/texture.h"

namespace gfx
{

	// Device texture, no support for mipmaps,
	class DTexture: public Resource
	{
	public:
		DTexture();
		DTexture(DTexture&&);
		DTexture(const DTexture&) = delete;
		void operator=(const DTexture&) = delete;
		void operator=(DTexture&&);
		~DTexture();

		void load(Stream&);

		void bind() const;
		static void bind0();

		void resize(TextureFormat format, int width, int height);
		void clear();

		void set(const Texture&);
		void upload(const Texture &src, const int2 &target_pos = int2(0, 0));
		void upload(const void *pixels, const int2 &dimensions, const int2 &target_pos);
		void download(Texture &target) const;
		void blit(DTexture &target, const IRect &src_rect, const int2 &target_pos) const;

		int width() const { return m_width; }
		int height() const { return m_height; }
		const int2 dimensions() const { return int2(m_width, m_height); }
		const TextureFormat format() const { return m_format; }

		int id() const { return m_id; }
		bool isValid() const { return m_id > 0; }

	private:
		int m_id;
		int m_width, m_height;
		TextureFormat m_format;
	};

	typedef Ptr<DTexture> PTexture;

	enum KeyId
	{
		Key_unknown		=-1,
		Key_space		=' ',
		Key_special		= 256,

		Key_esc,
		Key_f1,
		Key_f2,
		Key_f3,
		Key_f4,
		Key_f5,
		Key_f6,
		Key_f7,
		Key_f8,
		Key_f9,
		Key_f10,
		Key_f11,
		Key_f12,
		Key_up,
		Key_down,
		Key_left,
		Key_right,
		Key_lshift,
		Key_rshift,
		Key_lctrl,
		Key_rctrl,
		Key_lalt,
		Key_ralt,
		Key_tab,
		Key_enter,
		Key_backspace,
		Key_insert,
		Key_del,
		Key_pageup,
		Key_pagedown,
		Key_home,
		Key_end,
		Key_kp_0,
		Key_kp_1,
		Key_kp_2,
		Key_kp_3,
		Key_kp_4,
		Key_kp_5,
		Key_kp_6,
		Key_kp_7,
		Key_kp_8,
		Key_kp_9,
		Key_kp_divide,
		Key_kp_multiply,
		Key_kp_subtract,
		Key_kp_add,
		Key_kp_decimal,
		Key_kp_equal,
		Key_kp_enter,

		Key_count,
	};

	void createWindow(int2 size, bool fullscreen);
	void destroyWindow();
	void printDeviceInfo();

	bool pollEvents();
	void swapBuffers();

	void setWindowSize(int2 size);
	int2 getWindowSize();
	void setWindowPos(int2 pos);

	void setWindowTitle(const char *title);

	void grabMouse(bool);
	void showCursor(bool);

	char getCharPressed();

	// if key is pressed, after small delay, generates keypresses
	// every period frames
	bool isKeyDownAuto(int key, int period = 1);
	bool isKeyPressed(int);
	bool isKeyDown(int);
	bool isKeyUp(int);

	bool isMouseKeyPressed(int);
	bool isMouseKeyDown(int);
	bool isMouseKeyUp(int);

	int2 getMousePos();
	int2 getMouseMove();

	int getMouseWheelPos();
	int getMouseWheelMove();

	void lookAt(int2 pos);

	void drawQuad(int2 pos, int2 size, Color color = Color::white);
	inline void drawQuad(int x, int y, int w, int h, Color col = Color::white)
		{ drawQuad(int2(x, y), int2(w, h), col); }

	void drawQuad(int2 pos, int2 size, float2 uv0, float2 uv1, Color color = Color::white);

	void drawBBox(const FBox &wbox, Color col = Color::white, bool is_filled = false);
	void drawBBox(const IBox &wbox, Color col = Color::white, bool is_filled = false);

	void drawRect(const IRect &box, Color col = Color::white);
	void drawLine(int3 wp1, int3 wp2, Color color = Color::white);
	void drawLine(int2 p1, int2 p2, Color color = Color::white);

	void clear(Color color);

	enum BlendingMode {
		bmDisabled,
		bmNormal,
	};

	void setBlendingMode(BlendingMode mode);
	void setScissorRect(const IRect &rect);
	void setScissorTest(bool is_enabled);

}

#endif
