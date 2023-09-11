#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

typedef GLXContext (* glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, b32, int *);
typedef void (* glXSwapIntervalEXTProc)(Display *, GLXDrawable, int);
typedef int (* glXSwapIntervalSGIProc)(int);
typedef int (* glXSwapIntervalMESAProc)(unsigned int);

static Display *X11Display;
static GLXContext X11GLContext;
static Window X11Window;
static Cursor InvisibleCursor;
static Atom X11DeleteWindowEvent;

static void
PlatformOpenWindow(void)
{
	X11Display = XOpenDisplay(0);

	Window X11DefaultRootWindow = DefaultRootWindow(X11Display);
	int X11ScreenNumber = DefaultScreen(X11Display);

	long ForegroundColor = WhitePixel(X11Display, X11ScreenNumber);
	long BackgroundColor = BlackPixel(X11Display, X11ScreenNumber);

	X11Window = XCreateSimpleWindow(
		X11Display, X11DefaultRootWindow,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		WINDOW_BORDER_WIDTH, ForegroundColor, BackgroundColor);
	long EventMask = KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask;
	XSelectInput(X11Display, X11Window, EventMask);

	// NOTE(ariel) Fix window size.
	XSizeHints SizeHints = {0};
	SizeHints.flags = PMinSize | PMaxSize;
	SizeHints.min_width = WINDOW_WIDTH;
	SizeHints.max_width = WINDOW_WIDTH;
	SizeHints.min_height = WINDOW_HEIGHT;
	SizeHints.max_height = WINDOW_HEIGHT;
	XSetWMNormalHints(X11Display, X11Window, &SizeHints);

	// NOTE(ariel) Title window.
	XStoreName(X11Display, X11Window, "Falling Elements");

	// NOTE(ariel) Register event to close window properly when user clicks "x".
	X11DeleteWindowEvent = XInternAtom(X11Display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(X11Display, X11Window, &X11DeleteWindowEvent, 1);

	{
		s32 GLXMajorVersion = 0;
		s32 GLXMinorVersion = 0;
		glXQueryVersion(X11Display, &GLXMajorVersion, &GLXMinorVersion);
		Assert(GLXMajorVersion >= 1 && GLXMinorVersion >= 3);
	}

	s32 GLXVisualAttributes[] =
	{
		GLX_X_RENDERABLE, True,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		None,
	};
	s32 ViableFramebufferCount = 0;
	GLXFBConfig *ViableFramebuffers = glXChooseFBConfig(
		X11Display, X11ScreenNumber, GLXVisualAttributes, &ViableFramebufferCount);
	Assert(ViableFramebufferCount > 0);

	glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");
	Assert(glXCreateContextAttribsARB);

	s32 GLXContextAttributes[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None,
	};
	X11GLContext = glXCreateContextAttribsARB(X11Display, ViableFramebuffers[0], 0, True, GLXContextAttributes);
	Assert(X11GLContext);

	XMapWindow(X11Display, X11Window);
	glXMakeCurrent(X11Display, X11Window, X11GLContext);

	// NOTE(ariel) Enable VSync if support exists.
	{
		int EnableVSync = 1;

		glXSwapIntervalEXTProc glXSwapIntervalEXT = 0;
		glXSwapIntervalSGIProc glXSwapIntervalSGI = 0;
		glXSwapIntervalMESAProc glXSwapIntervalMESA = 0;

		glXSwapIntervalEXT = (glXSwapIntervalEXTProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");
		glXSwapIntervalSGI = (glXSwapIntervalSGIProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalSGI");
		glXSwapIntervalMESA = (glXSwapIntervalMESAProc)glXGetProcAddress((const GLubyte *)"glXSwapIntervalMESA");

		if (glXSwapIntervalEXT)
		{
			GLXDrawable X11Drawable = glXGetCurrentDrawable();
			glXSwapIntervalEXT(X11Display, X11Drawable, -1);
		}
		else if (glXSwapIntervalSGI)
		{
			glXSwapIntervalSGI(EnableVSync);
		}
		else if (glXSwapIntervalMESA)
		{
			glXSwapIntervalMESA(EnableVSync);
		}
	}

	// NOTE(ariel) Hide cursor by default.
	{
		XColor Color = {0};
		Color.red = Color.green = Color.blue = 0;

		Pixmap PixmapID = XCreatePixmap(X11Display, X11DefaultRootWindow, 1, 1, 1);
		Assert(PixmapID);

		InvisibleCursor = XCreatePixmapCursor(X11Display, PixmapID, PixmapID, &Color, &Color, 0, 0);
		XFreePixmap(X11Display, PixmapID);

		PlatformShowCursor(false);
	}

	XFree(ViableFramebuffers);
	XFlush(X11Display);

	LoadOpenGLExtensions();
}

static void
PlatformCloseWindow(void)
{
	glXDestroyContext(X11Display, X11GLContext);
	XDestroyWindow(X11Display, X11Window);
	XCloseDisplay(X11Display);
}

static void
PlatformSwapBuffers(void)
{
	glXSwapBuffers(X11Display, X11Window);
}

static void
PlatformShowCursor(b32 ShouldShowCursor)
{
	if (ShouldShowCursor)
	{
		XUndefineCursor(X11Display, X11Window);
	}
	else
	{
		XDefineCursor(X11Display, X11Window, InvisibleCursor);
	}
}

static void
PlatformHandleInput(void)
{
	for (s32 Count = XPending(X11Display); Count > 0; Count -= 1)
	{
		XEvent GeneralEvent = {0};
		XNextEvent(X11Display, &GeneralEvent);
		switch (GeneralEvent.type)
		{
			case KeyPress:
			{
				XKeyPressedEvent *Event = (XKeyPressedEvent *)&GeneralEvent;
				if (Event->keycode == XKeysymToKeycode(X11Display, XK_Escape))
				{
					Running = false;
				}
				break;
			}
			case ButtonPress:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				Input.MousePositionX = Event->x;
				Input.MousePositionY = Event->y;
				Input.PreviousMouseDown = Input.MouseDown;
				Input.MouseDown = true;
				break;
			}
			case ButtonRelease:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				Input.MousePositionX = Event->x;
				Input.MousePositionY = Event->y;
				Input.PreviousMouseDown = Input.MouseDown;
				Input.MouseDown = false;
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				Input.MousePositionX = Event->x;
				Input.MousePositionY = Event->y;
				break;
			}
			case EnterNotify:
			{
				Input.CursorIsInWindow = true;
				break;
			}
			case LeaveNotify:
			{
				Input.CursorIsInWindow = false;
				break;
			}
			case ClientMessage:
			{
				XClientMessageEvent *Event = (XClientMessageEvent *)&GeneralEvent;
				Atom Protocol = Event->data.l[0];
				if (Protocol == X11DeleteWindowEvent)
				{
					Running = false;
				}
				break;
			}
		}
	}
}

static inline u64
PlatformGetTime(void)
{
	u64 Nanoseconds = 0;
	struct timespec Now = {0};
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Nanoseconds += Now.tv_sec;
	Nanoseconds *= NANOSECONDS_PER_SECOND;
	Nanoseconds += Now.tv_nsec;
	return Nanoseconds;
}

static inline void
PlatformSleep(u64 DeltaTimeNS)
{
	static const u64 TARGET_FRAME_TIME_NS = NANOSECONDS_PER_SECOND / TARGET_FRAMES_PER_SECOND;
	s64 SleepTimeNS = TARGET_FRAME_TIME_NS - DeltaTimeNS;
	if (SleepTimeNS > 0)
	{
		struct timespec Time = {0};
		Time.tv_sec = SleepTimeNS / NANOSECONDS_PER_SECOND;
		Time.tv_nsec = SleepTimeNS % NANOSECONDS_PER_SECOND;
		nanosleep(&Time, 0);
	}
}
