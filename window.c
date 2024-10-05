/*
	window.c ~ RL
	Creates a window, loads OpenGL and parses input.
*/

#include "window.h"

#include <assert.h>
#include "camera.h"
#include "game.h"
#include "graphics.h"
#include <stdbool.h>
#include <stdio.h>
#include "util.h"

#include <Windows.h>
#include "opengl.h"

/* WGL_ARB_create_context */

#define WGL_CONTEXT_DEBUG_BIT_ARB			0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB			0x2093
#define WGL_CONTEXT_FLAGS_ARB				0x2094
#define ERROR_INVALID_VERSION_ARB			0x2095
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;

/* WGL_ARB_create_context_profile */

#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_PROFILE_ARB         0x2096

#define WINDOW_CAPTION L"Libraryless Minecraft"
#define TARGET_FRAMERATE 144

/* If res is false, prints Win32 error code and line where it failed and returns false. Otherwise, does nothing. */
#define DEBUG_PLATFORM_CALL(res)				(res || (printf(#res " failed with error code %i, at line %i.\n", GetLastError(), __LINE__), false))
/* Returns code if res is false */
#define DEBUG_PLATFORM_CALL_GUARD(res, code)	if (!DEBUG_PLATFORM_CALL(res)) return code;

#define GL_FUNCTION(type, name) type name = NULL;
GL_FUNCTION_LIST
#undef GL_FUNCTION

static int window_wx = 800, window_wy = 600;
static HWND window_handle;
static HDC window_dc;
static HGLRC window_glc;

static void* gl_load_func(HMODULE lib, const char* name)
{
	void* func = (void*)wglGetProcAddress(name);
	if (func == (void*)0 || func == (void*)1 || func == (void*)2 || func == (void*)3 || func == (void*)-1)
	{
		func = (void*)GetProcAddress(lib, name);
	}
	return func;
}

static bool gl_load(void)
{
	/* The call guards here return with a leak. This shouldn't be an issue, as this is a static function called only once and should exit the program prematurely if failed. */

	WNDCLASSEXW class =
	{
		.cbSize = sizeof class,
		.lpszClassName = L"LibrarylessMinecraft",
		.lpfnWndProc = DefWindowProcW,
	};

	DEBUG_PLATFORM_CALL_GUARD(RegisterClassExW(&class), false);

	HWND dummy_handle = CreateWindowExW(0, class.lpszClassName, L"", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
	DEBUG_PLATFORM_CALL_GUARD(dummy_handle, false);

	PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof pfd,
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cDepthBits = 24,
		.cStencilBits = 8,
	};

	HDC dummy_dc = GetDC(dummy_handle);
	DEBUG_PLATFORM_CALL_GUARD(dummy_dc, false);

	int pfi = ChoosePixelFormat(dummy_dc, &pfd);
	DEBUG_PLATFORM_CALL_GUARD(pfi, false);
	DEBUG_PLATFORM_CALL_GUARD(SetPixelFormat(dummy_dc, pfi, &pfd), false);

	HGLRC dummy_ctx = wglCreateContext(dummy_dc);
	DEBUG_PLATFORM_CALL_GUARD(dummy_ctx, false);
	DEBUG_PLATFORM_CALL_GUARD(wglMakeCurrent(dummy_dc, dummy_ctx), false);

	HMODULE opengl_handle = LoadLibraryW(L"OpenGL32.dll");
	int count = 0;

#define GL_FUNCTION(type, name) name = gl_load_func(opengl_handle, #name); count += !name;
	GL_FUNCTION_LIST
	GL_FUNCTION(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB)
#undef GL_FUNCTION

	DEBUG_PLATFORM_CALL(FreeLibrary(opengl_handle));
	
	DEBUG_PLATFORM_CALL(wglMakeCurrent(dummy_dc, NULL));
	DEBUG_PLATFORM_CALL(wglDeleteContext(dummy_ctx));
	DEBUG_PLATFORM_CALL(DestroyWindow(dummy_handle));
	DEBUG_PLATFORM_CALL(UnregisterClassW(class.lpszClassName, NULL));

	return count <= 0;
}

static pointi_t dims;

static LRESULT window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
		{
			glViewport(0, 0, LOWORD(lparam), HIWORD(lparam));
			dims = (pointi_t){ LOWORD(lparam), HIWORD(lparam) };
			camera_set_projection_properties(0.1F, 1000.0F, 90.0F);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

pointi_t window_get_dimensions(void)
{
	return dims;
}

static bool window_init(void)
{
	/* The call guards here return with a leak. This shouldn't be an issue, as this is a static function called only once and should exit the program prematurely if failed. */

	WNDCLASSEXW class =
	{
		.cbSize =			sizeof class,
		.lpszClassName =	L"LibrarylessMinecraft",
		.lpfnWndProc =		window_proc,
		.style =			CS_OWNDC | CS_VREDRAW | CS_HREDRAW
	};
	DEBUG_PLATFORM_CALL_GUARD(RegisterClassExW(&class), false);

	RECT region = { .right = window_wx, .bottom = window_wy };
	assert(AdjustWindowRectEx(&region, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW));
	window_handle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, class.lpszClassName, WINDOW_CAPTION, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, region.right - region.left, region.bottom - region.top, NULL, NULL, NULL, NULL);
	
	DEBUG_PLATFORM_CALL_GUARD(window_handle, false);

	PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof pfd,
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cDepthBits = 24,
		.cStencilBits = 8,
	};

	window_dc = GetDC(window_handle);
	DEBUG_PLATFORM_CALL_GUARD(window_dc, false);

	int pfi = ChoosePixelFormat(window_dc, &pfd);
	DEBUG_PLATFORM_CALL_GUARD(pfi, false);
	DEBUG_PLATFORM_CALL_GUARD(SetPixelFormat(window_dc, pfi, &pfd), false);
	
	int gl_attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_PROFILE_MASK_ARB, GL_CONTEXT_CORE_PROFILE_BIT,
		0
	};

	window_glc = wglCreateContextAttribsARB(window_dc, NULL, gl_attribs);
	DEBUG_PLATFORM_CALL_GUARD(window_glc, false);
	DEBUG_PLATFORM_CALL_GUARD(wglMakeCurrent(window_dc, window_glc), false);

	ShowWindow(window_handle, SW_NORMAL);
	UpdateWindow(window_handle);

	return true;
}

static pointi_t delta;
static pointi_t curr;
static bool lock_mouse = true;

static void window_mouse_update(void)
{
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	ScreenToClient(window_handle, &cursor_pos);
	pointi_t next = { cursor_pos.x, cursor_pos.y };
	delta = (pointi_t){ next.x - curr.x, next.y - curr.y };
	curr = next;

	if (window_input_clicked(INPUT_TOGGLE_MOUSE_FOCUS))
	{
		lock_mouse = !lock_mouse;
	}

	if (lock_mouse)
	{
		RECT dims;
		GetWindowRect(window_handle, &dims);
		POINT pt = { (dims.right - dims.left) / 2, (dims.bottom - dims.top) / 2 };
		curr = (pointi_t){ pt.x, pt.y };
		ClientToScreen(window_handle, &pt);
		SetCursorPos(pt.x, pt.y);
	}
}

pointi_t window_mouse_delta(void)
{
	return delta;
}

pointi_t window_mouse_position(void)
{
	return curr;
}

static bool state_array[INPUT_COUNT * 2];
static bool* state = state_array, *prev_state = state_array + INPUT_COUNT;

void window_input_update(void)
{
	if (GetForegroundWindow() != window_handle)
	{
		return;
	}

	bool* temp = prev_state;
	prev_state = state;
	state = temp;

	SHORT holding_ctrl = GetAsyncKeyState(VK_LCONTROL);

	state[INPUT_FORWARD] =		GetAsyncKeyState('W');
	state[INPUT_BACKWARD] =		GetAsyncKeyState('S');
	state[INPUT_LEFT] =			GetAsyncKeyState('A');
	state[INPUT_RIGHT] =		GetAsyncKeyState('D');
	state[INPUT_SNEAK] =		GetAsyncKeyState(VK_LSHIFT);
	state[INPUT_JUMP] =			GetAsyncKeyState(VK_SPACE);

	state[INPUT_TELEPORT_TO_SPAWN] = holding_ctrl && GetAsyncKeyState('T');

	state[INPUT_BREAK_BLOCK] =	GetAsyncKeyState(VK_LBUTTON);
	state[INPUT_PLACE_BLOCK] =	GetAsyncKeyState(VK_RBUTTON);
	state[INPUT_CYCLE_BLOCK_FORWARD] =	GetAsyncKeyState(VK_UP);
	state[INPUT_CYCLE_BLOCK_BACKWARD] =	GetAsyncKeyState(VK_DOWN);
	state[INPUT_QUEUE_BLOCK_INFO] =		GetAsyncKeyState(VK_MBUTTON);

	state[INPUT_TOGGLE_MOUSE_FOCUS] =	GetAsyncKeyState(VK_ESCAPE);
	state[INPUT_TOGGLE_WIREFRAME] =		holding_ctrl && GetAsyncKeyState('W');
}

bool window_input_down(input_t input)
{
	return state[input];
}

bool window_input_clicked(input_t input)
{
	return prev_state[input] && !state[input];
}

static LARGE_INTEGER frequency;
double window_time(void)
{
	LARGE_INTEGER curr;
	QueryPerformanceCounter(&curr);
	return (double)curr.QuadPart / frequency.QuadPart;
}

int main()
{
	DEBUG_PLATFORM_CALL_GUARD(gl_load(), 1);
	DEBUG_PLATFORM_CALL_GUARD(window_init(), 4);

	QueryPerformanceFrequency(&frequency);

	graphics_init();
	game_init();

	double fps_elapsed = 0.0, fps_samples = 0;

	/* This is technically less accurate than using LARGE_INTEGERs, but it shouldn't be that much worse */
	double curr, prev = window_time();

	bool running = true;
	while (running)
	{
		curr = window_time();
		MSG msg = { 0 };
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			running = running && msg.message != WM_QUIT;
		}

		if (GetForegroundWindow() == window_handle)
		{
			window_mouse_update();
		}

		double delta = curr - prev;
		game_frame((float)delta);
		graphics_debug_draw();

		SwapBuffers(window_dc);
		fps_elapsed += delta;
		fps_samples++;

		if (fps_elapsed > 1)
		{
			wchar_t title_buf[128];
			swprintf_s(title_buf, sizeof title_buf / sizeof * title_buf, WINDOW_CAPTION L", FPS: %i", (int)(1 / (fps_elapsed / fps_samples)));
			DEBUG_PLATFORM_CALL(SetWindowTextW(window_handle, title_buf));
			fps_elapsed = 0.0;
			fps_samples = 0;
		}
		prev = curr;
	}

	game_destroy();
	graphics_destroy();

	return 0;
}