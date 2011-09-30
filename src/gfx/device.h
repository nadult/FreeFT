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

		void Serialize(Serializer &sr);

		void Bind() const;
		static void Bind0();
		void Create(int mips);
		
		void SetSurface(const Texture &in);
		void GetSurface(Texture& out);

		void Free();
		int2 Size() const { return size; }
		int Width() const { return size.x; }
		int Height() const { return size.y; }
		TextureFormat GetFormat() const;

		int Id() const { return id; }
		bool IsValid() const { return id > 0; }

		static ResourceMgr<DTexture> mgr;

	private:
		void SetMip(int mip, TextureFormat fmt, void *pixels);
		void CreateMip(int mip, int width, int height, TextureFormat fmt);
		void UpdateMip(int mip, int x, int y, int w, int h, void *pixels, int pixelsInRow);

		DTexture(const DTexture&) { }
		void operator=(const DTexture&) { }

		int id, mips;
		int2 size;
	};

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

	void CreateWindow(int2 size, bool fullscreen);
	void DestroyWindow();

	bool PollEvents();
	void SwapBuffers();

	void SetWindowSize(int2 size);
	int2 GetWindowSize();

	void SetWindowTitle(const char *title);

	void GrabMouse(bool);
	void ShowCursor(bool);

	char GetCharDown();

	bool IsKeyPressed(int);
	bool IsKeyDown(int);
	bool IsKeyUp(int);

	bool IsMouseKeyPressed(int);
	bool IsMouseKeyDown(int);
	bool IsMouseKeyUp(int);

	int2 GetMousePos();
	int2 GetMouseMove();

	int GetMouseWheelPos();
	int GetMouseWheelMove();


	void DrawQuad(int2 pos, int2 size);
	inline void DrawQuad(int x, int y, int w, int h) { DrawQuad(int2(x, y), int2(w, h)); }

	void DrawQuad(int2 pos, int2 size, float2 uv0, float2 uv1);
	void DrawBBox(int2 pos, int3 size);
	void DrawLine(int2 pos, int3 p1, int3 p2, Color color);

	void Clear(Color color);

	enum BlendingMode {
		bmDisabled,
		bmNormal,
	};

	void SetBlendingMode(BlendingMode mode);

}

#endif
