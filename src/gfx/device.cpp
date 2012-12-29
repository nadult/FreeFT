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

	int  mouseDX, mouseDY;
	u32  key2Glfw[gfx::Key_last + 1];

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

		for(int n = 0; n < 256; n++)
			key2Glfw[n] = n;
		memset(key2Glfw + 256, 0, 4 * (Key_last + 1 - 256));

		key2Glfw[Key_space]       = GLFW_KEY_SPACE;
		key2Glfw[Key_special]     = GLFW_KEY_SPECIAL;
		key2Glfw[Key_esc]         = GLFW_KEY_ESC;
		key2Glfw[Key_f1]          = GLFW_KEY_F1;
		key2Glfw[Key_f2]          = GLFW_KEY_F2;
		key2Glfw[Key_f3]          = GLFW_KEY_F3;
		key2Glfw[Key_f4]          = GLFW_KEY_F4;
		key2Glfw[Key_f5]          = GLFW_KEY_F5;
		key2Glfw[Key_f6]          = GLFW_KEY_F6;
		key2Glfw[Key_f7]          = GLFW_KEY_F7;
		key2Glfw[Key_f8]          = GLFW_KEY_F8;
		key2Glfw[Key_f9]          = GLFW_KEY_F9;
		key2Glfw[Key_f10]         = GLFW_KEY_F10;
		key2Glfw[Key_f11]         = GLFW_KEY_F11;
		key2Glfw[Key_f12]         = GLFW_KEY_F12;
		key2Glfw[Key_up]          = GLFW_KEY_UP;
		key2Glfw[Key_down]        = GLFW_KEY_DOWN;
		key2Glfw[Key_left]        = GLFW_KEY_LEFT;
		key2Glfw[Key_right]       = GLFW_KEY_RIGHT;
		key2Glfw[Key_lshift]      = GLFW_KEY_LSHIFT;
		key2Glfw[Key_rshift]      = GLFW_KEY_RSHIFT;
		key2Glfw[Key_lctrl]       = GLFW_KEY_LCTRL;
		key2Glfw[Key_rctrl]       = GLFW_KEY_RCTRL;
		key2Glfw[Key_lalt]        = GLFW_KEY_LALT;
		key2Glfw[Key_ralt]        = GLFW_KEY_RALT;
		key2Glfw[Key_tab]         = GLFW_KEY_TAB;
		key2Glfw[Key_enter]       = GLFW_KEY_ENTER;
		key2Glfw[Key_backspace]   = GLFW_KEY_BACKSPACE;
		key2Glfw[Key_insert]      = GLFW_KEY_INSERT;
		key2Glfw[Key_del]         = GLFW_KEY_DEL;
		key2Glfw[Key_pageup]      = GLFW_KEY_PAGEUP;
		key2Glfw[Key_pagedown]    = GLFW_KEY_PAGEDOWN;
		key2Glfw[Key_home]        = GLFW_KEY_HOME;
		key2Glfw[Key_end]         = GLFW_KEY_END;
		key2Glfw[Key_kp_0]        = GLFW_KEY_KP_0;
		key2Glfw[Key_kp_1]        = GLFW_KEY_KP_1;
		key2Glfw[Key_kp_2]        = GLFW_KEY_KP_2;
		key2Glfw[Key_kp_3]        = GLFW_KEY_KP_3;
		key2Glfw[Key_kp_4]        = GLFW_KEY_KP_4;
		key2Glfw[Key_kp_5]        = GLFW_KEY_KP_5;
		key2Glfw[Key_kp_6]        = GLFW_KEY_KP_6;
		key2Glfw[Key_kp_7]        = GLFW_KEY_KP_7;
		key2Glfw[Key_kp_8]        = GLFW_KEY_KP_8;
		key2Glfw[Key_kp_9]        = GLFW_KEY_KP_9;
		key2Glfw[Key_kp_divide]   = GLFW_KEY_KP_DIVIDE;
		key2Glfw[Key_kp_multiply] = GLFW_KEY_KP_MULTIPLY;
		key2Glfw[Key_kp_subtract] = GLFW_KEY_KP_SUBTRACT;
		key2Glfw[Key_kp_add]      = GLFW_KEY_KP_ADD;
		key2Glfw[Key_kp_decimal]  = GLFW_KEY_KP_DECIMAL;
		key2Glfw[Key_kp_equal]    = GLFW_KEY_KP_EQUAL;
		key2Glfw[Key_kp_enter]    = GLFW_KEY_KP_ENTER;

		initDevice();
		initViewport(size);
		//TODO: initial mouse pos (until user moves it) is always 0,0

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

	void swapBuffers() { glfwSwapBuffers(); }
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

		return !s_want_close;
	}


	void setWindowSize(int2 size) { glfwSetWindowSize(size.x, size.y); }
	int2 getWindowSize() {
		int x, y;
		glfwGetWindowSize(&x, &y);
		return int2(x, y);
	}

	void setWindowTitle(const char *title) { glfwSetWindowTitle(title); }

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

	bool isKeyPressed(int k) { return activeInput.Key[key2Glfw[k]]; }
	bool isKeyDown(int k) { return activeInput.Key[key2Glfw[k]] && (!lastInput.Key[key2Glfw[k]]); }
	bool isKeyUp(int k) { return (!activeInput.Key[key2Glfw[k]]) && lastInput.Key[key2Glfw[k]]; }

	bool isMouseKeyPressed(int k) { return activeInput.MouseButton[k]; }
	bool isMouseKeyDown(int k) { return activeInput.MouseButton[k] && (!lastInput.MouseButton[k]); }
	bool isMouseKeyUp(int k) { return (!activeInput.MouseButton[k]) && lastInput.MouseButton[k]; }

	int2 getMousePos() { return int2(activeInput.MousePosX, activeInput.MousePosY); }
	int2 getMouseMove() { return int2(mouseDX, mouseDY); }

	int getMouseWheelPos() { return activeInput.WheelPos; }
	int getMouseWheelMove() { return activeInput.WheelPos - lastInput.WheelPos; }

}
