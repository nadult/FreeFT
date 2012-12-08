#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include "base.h"
#include "gfx/texture.h"

namespace gfx
{

	//! Device texture
	class DTexture: public Resource
	{
	public:
		DTexture();
		~DTexture();

		void serialize(Serializer &sr);

		void bind() const;
		static void bind0();
		void create(int mips);
		
		void setSurface(const Texture &in);
		void getSurface(Texture& out);

		void free();
		int2 size() const { return m_size; }
		int width() const { return m_size.x; }
		int height() const { return m_size.y; }
		TextureFormat format() const;

		int id() const { return m_id; }
		bool isValid() const { return m_id > 0; }

		static ResourceMgr<DTexture> mgr;

	private:
		void setMip(int mip, TextureFormat fmt, void *pixels);
		void createMip(int mip, int width, int height, TextureFormat fmt);
		void updateMip(int mip, int x, int y, int w, int h, void *pixels, int pixelsInRow);

		DTexture(const DTexture&) { }
		void operator=(const DTexture&) { }

		int m_id, m_mips;
		int2 m_size;
	};

	typedef Ptr<DTexture> PTexture;

	enum KeyId
	{
		Key_unknown		=-1,
		Key_space		=' ',
		Key_special		= 256,

		Key_esc			= (Key_special + 1),
		Key_f1			= (Key_special + 2),
		Key_f2			= (Key_special + 3),
		Key_f3			= (Key_special + 4),
		Key_f4			= (Key_special + 5),
		Key_f5			= (Key_special + 6),
		Key_f6			= (Key_special + 7),
		Key_f7			= (Key_special + 8),
		Key_f8			= (Key_special + 9),
		Key_f9			= (Key_special + 10),
		Key_f10			= (Key_special + 11),
		Key_f11			= (Key_special + 12),
		Key_f12			= (Key_special + 13),
		Key_up			= (Key_special + 14),
		Key_down		= (Key_special + 15),
		Key_left		= (Key_special + 16),
		Key_right		= (Key_special + 17),
		Key_lshift		= (Key_special + 18),
		Key_rshift		= (Key_special + 19),
		Key_lctrl		= (Key_special + 20),
		Key_rctrl		= (Key_special + 21),
		Key_lalt		= (Key_special + 22),
		Key_ralt		= (Key_special + 23),
		Key_tab			= (Key_special + 24),
		Key_enter		= (Key_special + 25),
		Key_backspace	= (Key_special + 26),
		Key_insert	 	= (Key_special + 27),
		Key_del		 	= (Key_special + 28),
		Key_pageup	 	= (Key_special + 29),
		Key_pagedown	= (Key_special + 30),
		Key_home		= (Key_special + 31),
		Key_end		 	= (Key_special + 32),
		Key_kp_0		= (Key_special + 33),
		Key_kp_1		= (Key_special + 34),
		Key_kp_2		= (Key_special + 35),
		Key_kp_3		= (Key_special + 36),
		Key_kp_4		= (Key_special + 37),
		Key_kp_5		= (Key_special + 38),
		Key_kp_6		= (Key_special + 39),
		Key_kp_7		= (Key_special + 40),
		Key_kp_8		= (Key_special + 41),
		Key_kp_9		= (Key_special + 42),
		Key_kp_divide	= (Key_special + 43),
		Key_kp_multiply = (Key_special + 44),
		Key_kp_subtract = (Key_special + 45),
		Key_kp_add	 	= (Key_special + 46),
		Key_kp_decimal	= (Key_special + 47),
		Key_kp_equal	= (Key_special + 48),
		Key_kp_enter	= (Key_special + 49),

		Key_last		= Key_kp_enter
	};

	void createWindow(int2 size, bool fullscreen);
	void destroyWindow();

	bool pollEvents();
	void swapBuffers();

	void setWindowSize(int2 size);
	int2 getWindowSize();

	void setWindowTitle(const char *title);

	void grabMouse(bool);
	void showCursor(bool);

	char getCharPressed();

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
