/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gfx/device.h"
#include "gfx/texture_cache.h"
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
	u32  s_key_map[gfx::Key::count];

	bool s_want_close = 0;
	bool s_is_initialized = 0;
	bool s_has_window = 0;

	int GLFWCALL CloseWindowHandle() {
		s_want_close = 1;
		return GL_FALSE;
	}

}

namespace gfx
{

	void loadExtensions();
	void initViewport(int2 size);

	void initDevice() {
		ASSERT(!s_is_initialized);
		if(!glfwInit())
			THROW("Error while initializing GLFW");
		glfwDisable(GLFW_AUTO_POLL_EVENTS);
		s_is_initialized = true;
		s_last_time = -1.0;

		atexit(freeDevice);
	}

	void freeDevice() {
		if(s_has_window)
			destroyWindow();
		if(s_is_initialized) {
			glfwTerminate();
			s_is_initialized = 0;
		}
	}

	void createWindow(int2 size, bool full) {
		ASSERT(s_is_initialized);
		ASSERT(!s_has_window);

		glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
		if(!glfwOpenWindow(size.x, size.y, 8, 8, 8, 8, 24, 8, full ? GLFW_FULLSCREEN : GLFW_WINDOW))
			THROW("Error while initializing window with glfwOpenGLWindow");

		glfwSwapInterval(1);
		glfwSetWindowPos(0, 24);

		s_want_close = 0;
		glfwSetWindowCloseCallback(CloseWindowHandle);

		memset(&lastInput, 0, sizeof(lastInput));
		memset(&activeInput, 0, sizeof(activeInput));
		glfwGetMousePos(&activeInput.MousePosX, &activeInput.MousePosY);

		for(int n = 0; n < Key::special; n++)
			s_key_map[n] = n;
		memset(s_key_map + Key::special, 0, (Key::count - Key::special) * sizeof(s_key_map[0]));

		for(int n = 0; n < Key::count; n++)
			s_time_pressed[n] = -1.0;

		s_key_map[Key::space]       = GLFW_KEY_SPACE;
		s_key_map[Key::special]     = GLFW_KEY_SPECIAL;
		s_key_map[Key::esc]         = GLFW_KEY_ESC;
		s_key_map[Key::f1]          = GLFW_KEY_F1;
		s_key_map[Key::f2]          = GLFW_KEY_F2;
		s_key_map[Key::f3]          = GLFW_KEY_F3;
		s_key_map[Key::f4]          = GLFW_KEY_F4;
		s_key_map[Key::f5]          = GLFW_KEY_F5;
		s_key_map[Key::f6]          = GLFW_KEY_F6;
		s_key_map[Key::f7]          = GLFW_KEY_F7;
		s_key_map[Key::f8]          = GLFW_KEY_F8;
		s_key_map[Key::f9]          = GLFW_KEY_F9;
		s_key_map[Key::f10]         = GLFW_KEY_F10;
		s_key_map[Key::f11]         = GLFW_KEY_F11;
		s_key_map[Key::f12]         = GLFW_KEY_F12;
		s_key_map[Key::up]          = GLFW_KEY_UP;
		s_key_map[Key::down]        = GLFW_KEY_DOWN;
		s_key_map[Key::left]        = GLFW_KEY_LEFT;
		s_key_map[Key::right]       = GLFW_KEY_RIGHT;
		s_key_map[Key::lshift]      = GLFW_KEY_LSHIFT;
		s_key_map[Key::rshift]      = GLFW_KEY_RSHIFT;
		s_key_map[Key::lctrl]       = GLFW_KEY_LCTRL;
		s_key_map[Key::rctrl]       = GLFW_KEY_RCTRL;
		s_key_map[Key::lalt]        = GLFW_KEY_LALT;
		s_key_map[Key::ralt]        = GLFW_KEY_RALT;
		s_key_map[Key::tab]         = GLFW_KEY_TAB;
		s_key_map[Key::enter]       = GLFW_KEY_ENTER;
		s_key_map[Key::backspace]   = GLFW_KEY_BACKSPACE;
		s_key_map[Key::insert]      = GLFW_KEY_INSERT;
		s_key_map[Key::del]         = GLFW_KEY_DEL;
		s_key_map[Key::pageup]      = GLFW_KEY_PAGEUP;
		s_key_map[Key::pagedown]    = GLFW_KEY_PAGEDOWN;
		s_key_map[Key::home]        = GLFW_KEY_HOME;
		s_key_map[Key::end]         = GLFW_KEY_END;
		s_key_map[Key::kp_0]        = GLFW_KEY_KP_0;
		s_key_map[Key::kp_1]        = GLFW_KEY_KP_1;
		s_key_map[Key::kp_2]        = GLFW_KEY_KP_2;
		s_key_map[Key::kp_3]        = GLFW_KEY_KP_3;
		s_key_map[Key::kp_4]        = GLFW_KEY_KP_4;
		s_key_map[Key::kp_5]        = GLFW_KEY_KP_5;
		s_key_map[Key::kp_6]        = GLFW_KEY_KP_6;
		s_key_map[Key::kp_7]        = GLFW_KEY_KP_7;
		s_key_map[Key::kp_8]        = GLFW_KEY_KP_8;
		s_key_map[Key::kp_9]        = GLFW_KEY_KP_9;
		s_key_map[Key::kp_divide]   = GLFW_KEY_KP_DIVIDE;
		s_key_map[Key::kp_multiply] = GLFW_KEY_KP_MULTIPLY;
		s_key_map[Key::kp_subtract] = GLFW_KEY_KP_SUBTRACT;
		s_key_map[Key::kp_add]      = GLFW_KEY_KP_ADD;
		s_key_map[Key::kp_decimal]  = GLFW_KEY_KP_DECIMAL;
		s_key_map[Key::kp_equal]    = GLFW_KEY_KP_EQUAL;
		s_key_map[Key::kp_enter]    = GLFW_KEY_KP_ENTER;

		loadExtensions();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(0);

		initViewport(size);
		//TODO: initial mouse pos (until user moves it) is always 0,0
			
		glfwPollEvents();
		setBlendingMode(bmNormal);

		s_has_window = true;
		atexit(destroyWindow);
	}

	void destroyWindow() {
		if(s_has_window) {
			glfwCloseWindow();
			s_has_window = false;
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
	
	double targetFrameTime() {
		return 1.0 / 60.0;
	}

	void tick() {
		double time_diff = getTime() - s_last_time;

		if(s_has_window) {
			TextureCache::main_cache.nextFrame();
			glfwSwapBuffers();
		}
		else {
			double target_diff = targetFrameTime();
			double target_time = s_last_time + target_diff;

#ifdef _WIN32
			//TODO: check if this is enough
			double busy_sleep = 0.001;
#else
			double busy_sleep = 0.0002;
#endif

			if(time_diff < target_diff) {
				sleep(target_diff - time_diff - busy_sleep);
				while(getTime() < target_time)
					;
			}
		}
		
		s_last_time = getTime();
	}

	bool pollEvents() {
		if(s_has_window)
			glfwPollEvents();

		lastInput = activeInput;
		memcpy(&activeInput, &_glfwInput, sizeof(_glfwInput));
		glfwGetMousePos(&activeInput.MousePosX, &activeInput.MousePosY);

		mouseDX = activeInput.MousePosX - lastInput.MousePosX;
		mouseDY = activeInput.MousePosY - lastInput.MousePosY;

		double time = getTime();
		for(int n = 0; n < GLFW_KEY_LAST + 1; n++) {
			char previous = lastInput.Key[n];
			char current  = activeInput.Key[n];
			
			if(current && !previous)
				s_time_pressed[n] = time;
			else if(!current)
				s_time_pressed[n] = -1.0;
		}
		s_clock++;

		return !s_want_close;
	}

	const int2 getWindowSize() {
		DASSERT(s_has_window);

		int2 out;
		glfwGetWindowSize(&out.x, &out.y);
		return out;
	}

	void setWindowPos(const int2 &pos) {
		DASSERT(s_has_window);

		glfwSetWindowPos(pos.x, pos.y);
	}

	void setWindowTitle(const char *title) {
		DASSERT(s_has_window);

		glfwSetWindowTitle(title);
		glfwPollEvents(); //TODO: remove, set window on creation
	}

	void grabMouse(bool grab) {
		if(grab)
			glfwDisable(GLFW_MOUSE_CURSOR);
		else
			glfwEnable(GLFW_MOUSE_CURSOR);
	}

	void showCursor(bool flag) {
		if(flag)
			glfwEnable(GLFW_MOUSE_CURSOR);
		else
			glfwDisable(GLFW_MOUSE_CURSOR);
	}

	char getCharPressed() {
		if(isKeyPressed(Key::space))
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

		bool shift = isKeyPressed(Key::lshift) || isKeyPressed(Key::rshift); //TODO: capslock

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

	int2 getMousePos() {
		return int2(activeInput.MousePosX, activeInput.MousePosY);
	}

	int2 getMouseMove() {
		return int2(mouseDX, mouseDY);
	}

	int getMouseWheelPos() {
		return activeInput.WheelPos;
	}

	int getMouseWheelMove() {
		return activeInput.WheelPos - lastInput.WheelPos;
	}

	using io::InputEvent;

	const vector<InputEvent> generateInputEvents() {
		vector<InputEvent> out;
		
		const int2 mouse_pos = getMousePos(), mouse_move = getMouseMove();

		if(mouse_move != int2(0, 0))
			out.push_back({InputEvent::mouse_moved, 0, mouse_move});
		if(getMouseWheelMove())
			out.push_back({InputEvent::mouse_wheel, 0, int2(getMouseWheelMove(), 0)});

		for(int n = 0; n < Key::count; n++) {
			if(isKeyDown(n))
				out.push_back({InputEvent::key_down, n, 0});
			if(isKeyUp(n))
				out.push_back({InputEvent::key_up, n, 0});
			if(isKeyPressed(n))
				out.push_back({InputEvent::key_pressed, n, 0});
			if(isKeyDownAuto(n, 1))
				out.push_back({InputEvent::key_down_auto, n, s_clock});
		}

		for(int mk = 0; mk < 3; mk++) {
			if(isMouseKeyDown(mk))
				out.push_back({InputEvent::mouse_key_down, mk, mouse_move});
			if(isMouseKeyUp(mk))
				out.push_back({InputEvent::mouse_key_up, mk, mouse_move});
			if(isMouseKeyPressed(mk))
				out.push_back({InputEvent::mouse_key_pressed, mk, mouse_move});
		}
		
		int modifiers =
			(isKeyDown(Key::lshift)? InputEvent::mod_lshift : 0) |
			(isKeyDown(Key::rshift)? InputEvent::mod_rshift : 0) |
			(isKeyDown(Key::lctrl)? InputEvent::mod_lctrl  : 0) |
			(isKeyDown(Key::lalt)? InputEvent::mod_lalt   : 0);
		for(int n = 0; n < (int)out.size(); n++) {
			out[n].setModifiers(modifiers);
			out[n].setMousePos(mouse_pos);
		}
		
		return out;
	}

}
