/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/device.h"
#include <memory.h>
#include <GL/glfw.h>

extern "C"
{
	extern struct
	{
		int  MousePosX, MousePosY;
		int  WheelPos;
		char MouseButton[GLFW_MOUSE_BUTTON_LAST + 1];
		char Key[GLFW_KEY_LAST + 1];
		int  LastChar;
		int  StickyKeys;
		int  StickyMouseButtons;
		int  KeyRepeat;
		int  MouseMoved, OldMouseX, OldMouseY;
	} _glfwInput;
	//TODO: make proper input handling
}

namespace
{

	struct InputState
	{
		int  MousePosX, MousePosY;
		int  WheelPos;
		char MouseButton[GLFW_MOUSE_BUTTON_LAST + 1];
		char Key[GLFW_KEY_LAST + 1];
		int  LastChar;
		int  StickyKeys;
		int  StickyMouseButtons;
		int  KeyRepeat;
		int  MouseMoved, OldMouseX, OldMouseY;
	} lastInput, activeInput;

	double s_time_pressed[GLFW_KEY_LAST + 1];
	double s_last_time = -1.0;
	int s_clock = 0;
	const double s_press_delay = 0.4;

	int  mouseDX, mouseDY;
	u32  s_key_map[gfx::Key_count];

	bool s_want_close = 0, s_is_created = 0;

	int GLFWCALL CloseWindowHandle() {
		s_want_close = 1;
		return GL_FALSE;
	}

}

namespace gfx
{

	void initViewport(int2 size);

	static void initDevice() {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(0);
	}


	void createWindow(int2 size, bool full) {
		if(s_is_created)
			THROW("Trying to create more than one glfw window");
		if(!glfwInit())
			THROW("Error while initializing GLFW");

		glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);

		if(!glfwOpenWindow(size.x, size.y, 8, 8, 8, 8, 24, 8, full ? GLFW_FULLSCREEN : GLFW_WINDOW))
			THROW("Error while initializing window with glfwOpenGLWindow");

		glfwDisable(GLFW_AUTO_POLL_EVENTS);
		glfwSwapInterval(1);
		glfwSetWindowPos(0, 24);

		s_want_close = 0;
		glfwSetWindowCloseCallback(CloseWindowHandle);

		memset(&lastInput, 0, sizeof(lastInput));
		memset(&activeInput, 0, sizeof(activeInput));

		for(int n = 0; n < Key_special; n++)
			s_key_map[n] = n;
		memset(s_key_map + Key_special, 0, (Key_count - Key_special) * sizeof(s_key_map[0]));

		for(int n = 0; n < Key_count; n++)
			s_time_pressed[n] = -1.0;

		s_key_map[Key_space]       = GLFW_KEY_SPACE;
		s_key_map[Key_special]     = GLFW_KEY_SPECIAL;
		s_key_map[Key_esc]         = GLFW_KEY_ESC;
		s_key_map[Key_f1]          = GLFW_KEY_F1;
		s_key_map[Key_f2]          = GLFW_KEY_F2;
		s_key_map[Key_f3]          = GLFW_KEY_F3;
		s_key_map[Key_f4]          = GLFW_KEY_F4;
		s_key_map[Key_f5]          = GLFW_KEY_F5;
		s_key_map[Key_f6]          = GLFW_KEY_F6;
		s_key_map[Key_f7]          = GLFW_KEY_F7;
		s_key_map[Key_f8]          = GLFW_KEY_F8;
		s_key_map[Key_f9]          = GLFW_KEY_F9;
		s_key_map[Key_f10]         = GLFW_KEY_F10;
		s_key_map[Key_f11]         = GLFW_KEY_F11;
		s_key_map[Key_f12]         = GLFW_KEY_F12;
		s_key_map[Key_up]          = GLFW_KEY_UP;
		s_key_map[Key_down]        = GLFW_KEY_DOWN;
		s_key_map[Key_left]        = GLFW_KEY_LEFT;
		s_key_map[Key_right]       = GLFW_KEY_RIGHT;
		s_key_map[Key_lshift]      = GLFW_KEY_LSHIFT;
		s_key_map[Key_rshift]      = GLFW_KEY_RSHIFT;
		s_key_map[Key_lctrl]       = GLFW_KEY_LCTRL;
		s_key_map[Key_rctrl]       = GLFW_KEY_RCTRL;
		s_key_map[Key_lalt]        = GLFW_KEY_LALT;
		s_key_map[Key_ralt]        = GLFW_KEY_RALT;
		s_key_map[Key_tab]         = GLFW_KEY_TAB;
		s_key_map[Key_enter]       = GLFW_KEY_ENTER;
		s_key_map[Key_backspace]   = GLFW_KEY_BACKSPACE;
		s_key_map[Key_insert]      = GLFW_KEY_INSERT;
		s_key_map[Key_del]         = GLFW_KEY_DEL;
		s_key_map[Key_pageup]      = GLFW_KEY_PAGEUP;
		s_key_map[Key_pagedown]    = GLFW_KEY_PAGEDOWN;
		s_key_map[Key_home]        = GLFW_KEY_HOME;
		s_key_map[Key_end]         = GLFW_KEY_END;
		s_key_map[Key_kp_0]        = GLFW_KEY_KP_0;
		s_key_map[Key_kp_1]        = GLFW_KEY_KP_1;
		s_key_map[Key_kp_2]        = GLFW_KEY_KP_2;
		s_key_map[Key_kp_3]        = GLFW_KEY_KP_3;
		s_key_map[Key_kp_4]        = GLFW_KEY_KP_4;
		s_key_map[Key_kp_5]        = GLFW_KEY_KP_5;
		s_key_map[Key_kp_6]        = GLFW_KEY_KP_6;
		s_key_map[Key_kp_7]        = GLFW_KEY_KP_7;
		s_key_map[Key_kp_8]        = GLFW_KEY_KP_8;
		s_key_map[Key_kp_9]        = GLFW_KEY_KP_9;
		s_key_map[Key_kp_divide]   = GLFW_KEY_KP_DIVIDE;
		s_key_map[Key_kp_multiply] = GLFW_KEY_KP_MULTIPLY;
		s_key_map[Key_kp_subtract] = GLFW_KEY_KP_SUBTRACT;
		s_key_map[Key_kp_add]      = GLFW_KEY_KP_ADD;
		s_key_map[Key_kp_decimal]  = GLFW_KEY_KP_DECIMAL;
		s_key_map[Key_kp_equal]    = GLFW_KEY_KP_EQUAL;
		s_key_map[Key_kp_enter]    = GLFW_KEY_KP_ENTER;

		initDevice();
		initViewport(size);
		//TODO: initial mouse pos (until user moves it) is always 0,0
			
		glfwPollEvents();

		s_is_created = 1;
	}

	void destroyWindow() {
		if(s_is_created) {
			glfwCloseWindow();
			glfwTerminate();
			s_is_created = 0;
		}
	}

	void printDeviceInfo() {
		int max_tex_size;

		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
		const char *vendor = (const char*)glGetString(GL_VENDOR);
		const char *renderer = (const char*)glGetString(GL_RENDERER);

		printf(	"Opengl info\n"
				"Vendor: %s\nRenderer: %s\n"
				"Maximum texture size: %d\n",
				vendor, renderer, max_tex_size);
	}

	void swapBuffers() {
		glfwSwapBuffers();
	}

	bool pollEvents() {
		for(;;) {
			glfwPollEvents();

			if(glfwGetWindowParam(GLFW_ICONIFIED)) {
				sleep(0.05);
				continue;
			}
			break;
		}
		int width, height;
		glfwGetWindowSize(&width, &height);

		lastInput = activeInput;
		memcpy(&activeInput, &_glfwInput, sizeof(_glfwInput));

		mouseDX = activeInput.MousePosX - lastInput.MousePosX;
		mouseDY = activeInput.MousePosY - lastInput.MousePosY;
		activeInput.MousePosX = clamp(activeInput.MousePosX, 0, width - 1);
		activeInput.MousePosY = clamp(activeInput.MousePosY, 0, height - 1);
		//glfwSetMousePos(activeInput.MousePosX, activeInput.MousePosY);

		double time = getTime();
		for(int n = 0; n < GLFW_KEY_LAST + 1; n++) {
			char previous = lastInput.Key[n];
			char current  = activeInput.Key[n];
			
			if(current && !previous)
				s_time_pressed[n] = time;
			else if(!current)
				s_time_pressed[n] = -1.0;
		}
		s_last_time = time;
		s_clock++;

		return !s_want_close;
	}


	void setWindowSize(int2 size) { glfwSetWindowSize(size.x, size.y); }
	int2 getWindowSize() {
		int x, y;
		glfwGetWindowSize(&x, &y);
		return int2(x, y);
	}

	void setWindowPos(int2 pos) {
		glfwSetWindowPos(pos.x, pos.y);
	}

	void setWindowTitle(const char *title) {
		glfwSetWindowTitle(title);
		glfwPollEvents(); //TODO: remove, set window on creation
	}

	void grabMouse(bool grab)
	{
		if(grab)
			glfwDisable(GLFW_MOUSE_CURSOR);
		else
			glfwEnable(GLFW_MOUSE_CURSOR);
	}

	void showCursor(bool flag)
	{
		if(flag)
			glfwEnable(GLFW_MOUSE_CURSOR);
		else
			glfwDisable(GLFW_MOUSE_CURSOR);
	}

	char getCharPressed() {
		if(isKeyPressed(Key_space))
			return ' ';

		char numerics_s[11] = ")!@#$%^&*(";
		char map[][2] = {
			{ '-', '_' },
			{ '`', '~' },
			{ '=', '+' },
			{ '[', '{' },
			{ ']', '}' },
			{ ';', ':' },
			{ '\'', '"' },
			{ ',', '<' },
			{ '.', '>' },
			{ '/', '?' },
			{ '\\', '|' },
		};

		bool shift = isKeyPressed(Key_lshift) || isKeyPressed(Key_rshift); //TODO: capslock

		for(int i = 0; i < (int)sizeof(map) / 2; i++)
			if(isKeyPressed(map[i][0]))
				return map[i][shift? 1 : 0];

		for(int k = 32; k < 128; k++) {
			if(!isKeyPressed(k))
				continue;
			if(k >= 'A' && k <= 'Z')
				return shift ? k : k - 'A' + 'a';
			if(k >= '0' && k <= '9')
				return shift? numerics_s[k - '0'] : k;
		}

		return 0;
	}

	bool isKeyPressed(int k) {
		return activeInput.Key[s_key_map[k]];
	}

	bool isKeyDown(int k) {
		return activeInput.Key[s_key_map[k]] && (!lastInput.Key[s_key_map[k]]);
	}

	bool isKeyDownAuto(int k, int period) {
		int id = s_key_map[k];
		if(!activeInput.Key[id])
			return false;
		return !lastInput.Key[id] || (s_last_time - s_time_pressed[id] > s_press_delay && s_clock % period == 0);
	}

	bool isKeyUp(int k) {
		return (!activeInput.Key[s_key_map[k]]) && lastInput.Key[s_key_map[k]];
	}

	bool isMouseKeyPressed(int k) { return activeInput.MouseButton[k]; }
	bool isMouseKeyDown(int k) { return activeInput.MouseButton[k] && (!lastInput.MouseButton[k]); }
	bool isMouseKeyUp(int k) { return (!activeInput.MouseButton[k]) && lastInput.MouseButton[k]; }

	int2 getMousePos() { return int2(activeInput.MousePosX, activeInput.MousePosY); }
	int2 getMouseMove() { return int2(mouseDX, mouseDY); }

	int getMouseWheelPos() { return activeInput.WheelPos; }
	int getMouseWheelMove() { return activeInput.WheelPos - lastInput.WheelPos; }

}
