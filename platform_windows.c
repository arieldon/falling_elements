#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HWND Win32Window;
static HDC Win32DeviceContext;
static HGLRC Win32GraphicsContext;

static LRESULT CALLBACK
WindowProcedure(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				Running = false;
			}
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			Input.PreviousMouseDown = Input.MouseDown; // FIXME(ariel) Is that at all useful? It seems totally useless, even for the menu.
			Input.MouseDown = true;
			break;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			Input.PreviousMouseDown = Input.MouseDown; // FIXME(ariel) Is that at all useful? It seems totally useless, even for the menu.
			Input.MouseDown = false;
			break;
		}
		case WM_MOUSEMOVE:
		{
			TRACKMOUSEEVENT TrackMouseEventParameter = {0};
			TrackMouseEventParameter.cbSize = sizeof(TrackMouseEventParameter);
			TrackMouseEventParameter.dwFlags = TME_LEAVE;
			TrackMouseEventParameter.hwndTrack = Win32Window;
			TrackMouseEventParameter.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&TrackMouseEventParameter);

			// FIXME(ariel) In Windows, the coordinates along the y-axis seem to be
			// offset for some reason. The addition below adjusts for this to match
			// coordinates between Linux and Windows.
			Input.CursorIsInWindow = true;
			Input.MousePositionX = (s16)(lParam >> 0x00);
			Input.MousePositionY = (s16)(lParam >> 0x10) + 24;
			break;
		}
		case WM_MOUSELEAVE:
		{
			Input.CursorIsInWindow = false;
			break;
		}
		// TODO(ariel) What's the diffrence between `WM_CLOSE` and `WM_DESTROY`?
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			Running = false;
			break;
		}
		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
			break;
		}
	}

	return Result;
}

static void
PlatformOpenWindow(void)
{
	// NOTE(ariel) Get name of executable used to create the calling process.
	HINSTANCE Instance = GetModuleHandle(0);
	AssertAlways(Instance);

	// NOTE(ariel) Create a window.
	{
		char *ClassName = "FallingElementsClass";
		char *WindowName = "Falling Elements";

		WNDCLASSA WindowClass = {0};
		WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		WindowClass.lpfnWndProc = WindowProcedure;
		WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
		WindowClass.hInstance = Instance;
		WindowClass.lpszClassName = ClassName;

		ATOM Atom = RegisterClassA(&WindowClass);
		AssertAlways(Atom);

		s32 X = CW_USEDEFAULT;
		s32 Y = CW_USEDEFAULT;
		s32 ExtendedWindowStyle = 0;
		s32 Style = WS_VISIBLE;

		HWND WindowParent = 0;
		HMENU Menu = 0;
		LPVOID lpParam = 0;

		Win32Window = CreateWindowExA(
			ExtendedWindowStyle,
			ClassName, WindowName,
			Style, X, Y, WINDOW_WIDTH, WINDOW_HEIGHT,
			WindowParent, Menu, Instance, lpParam);
		AssertAlways(Win32Window);

		Win32DeviceContext = GetDC(Win32Window);
		AssertAlways(Win32DeviceContext);
	}

	// NOTE(ariel) Create an OpenGL context associated with the window.
	{
		PIXELFORMATDESCRIPTOR PixelFormatDescriptor = { 0 };

		PixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		PixelFormatDescriptor.nVersion = 1;
		PixelFormatDescriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		PixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
		PixelFormatDescriptor.cColorBits = 32;
		PixelFormatDescriptor.cDepthBits = 32; // FIXME(ariel) Switch to 24-bit depth buffer?
		PixelFormatDescriptor.dwLayerMask = PFD_MAIN_PLANE;

		s32 PixelFormat = ChoosePixelFormat(Win32DeviceContext, &PixelFormatDescriptor);
		AssertAlways(PixelFormat);

		b32 Success = SetPixelFormat(Win32DeviceContext, PixelFormat, &PixelFormatDescriptor);
		AssertAlways(Success);

		Win32GraphicsContext = wglCreateContext(Win32DeviceContext);
		AssertAlways(Win32GraphicsContext);

		wglMakeCurrent(Win32DeviceContext, Win32GraphicsContext);
	}

	ShowWindow(Win32Window, SW_SHOW);

	// TODO(ariel) Currently this loads an unspecified version of OpenGL -- and
	// it doesn't match the version on Linux on my machine. There's a more
	// complex way to create an OpenGL context on Windows that allows the
	// programmer to configure many more options.
	LoadOpenGLExtensions();
}

static void
PlatformCloseWindow(void)
{
	wglMakeCurrent(0, 0);
	wglDeleteContext(Win32GraphicsContext);
	ReleaseDC(Win32Window, Win32DeviceContext);
	DestroyWindow(Win32Window);
}

static void
PlatformSwapBuffers(void)
{
	SwapBuffers(Win32DeviceContext);
}

static void
PlatformShowCursor(b32 ShouldShowCursor)
{
	static s32 CursorDisplayCount;
	if (CursorDisplayCount == 0 & !ShouldShowCursor)
	{
		CursorDisplayCount = ShowCursor(false);
	}
	else if (CursorDisplayCount == -1 & ShouldShowCursor)
	{
		CursorDisplayCount = ShowCursor(true);
	}
	Assert(CursorDisplayCount == 0 | CursorDisplayCount == -1);
}

static void
PlatformHandleInput(void)
{
	MSG Message = {0};
	while (PeekMessage(&Message, Win32Window, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
}

static inline u64
PlatformGetTime(void)
{
	LARGE_INTEGER Value = {0};
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

static inline void
PlatformSleep(u64 DeltaTimeMS)
{
	static const u64 TARGET_FRAME_TIME_MS = MILLISECONDS_PER_SECOND / TARGET_FRAMES_PER_SECOND;
	s64 SleepTimeMS = TARGET_FRAME_TIME_MS - DeltaTimeMS;
	if (SleepTimeMS > 0)
	{
		DWORD SleepTime = (DWORD)SleepTimeMS;
		Assert(SleepTime <= SleepTimeMS);
		Sleep(SleepTime);
	}
}
