#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

#include "base.h"

enum
{
	WINDOW_WIDTH = 1920,
	WINDOW_HEIGHT = 1080,
	WINDOW_BORDER_WIDTH = 0,
};
StaticAssert(WINDOW_WIDTH > 0);
StaticAssert(WINDOW_HEIGHT > 0);

enum { FRAMES_PER_SECOND = 60 };

static Display *X11Display;
static Window X11Window;
static GC X11GraphicsContext;
static XImage *X11Image;
static Atom X11DeleteWindowEvent;

static u32 Buffer[WINDOW_WIDTH * WINDOW_HEIGHT];
static b32 Running = true;

static void
OpenWindow(void)
{
	X11Display = XOpenDisplay(0);

	Window X11DefaultRootWindow = DefaultRootWindow(X11Display);
	int X11ScreenNumber = DefaultScreen(X11Display);

	int ForegroundColor = WhitePixel(X11Display, X11ScreenNumber);
	int BackgroundColor = BlackPixel(X11Display, X11ScreenNumber);

	X11Window = XCreateSimpleWindow(
		X11Display, X11DefaultRootWindow,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		WINDOW_BORDER_WIDTH, ForegroundColor, BackgroundColor);
	long EventMask = KeyPressMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask;
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
	XStoreName(X11Display, X11Window, "Sand");

	X11GraphicsContext = XCreateGC(X11Display, X11Window, 0, 0);
	XSetForeground(X11Display, X11GraphicsContext, ForegroundColor);
	XSetBackground(X11Display, X11GraphicsContext, BackgroundColor);

	int Depth = XDefaultDepth(X11Display, X11ScreenNumber);
	Visual *X11Visual = XDefaultVisual(X11Display, X11ScreenNumber);
	X11Image = XCreateImage(X11Display, X11Visual,
		Depth, ZPixmap, 0,
		(char *)Buffer, WINDOW_WIDTH, WINDOW_HEIGHT,
		32, 0);

	// NOTE(ariel) Register event to close window properly when user clicks "x".
	X11DeleteWindowEvent = XInternAtom(X11Display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(X11Display, X11Window, &X11DeleteWindowEvent, 1);

	XMapWindow(X11Display, X11Window);
	XFlush(X11Display);
}

static void
CloseWindow(void)
{
	XDestroyWindow(X11Display, X11Window);
	XCloseDisplay(X11Display);
}

static void
HandleInput(void)
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
			case ButtonRelease:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				if (Event->button == Button1)
				{
					printf("(%d) %d, %d\n", Event->button, Event->x, Event->y);
				}
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				printf("%d, %d\n", Event->x, Event->y);
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

static void
ClearBuffer(void)
{
	for (s32 Y = 0; Y < WINDOW_HEIGHT; Y += 1)
	{
		for (s32 X = 0; X < WINDOW_WIDTH; X += 1)
		{
			Buffer[WINDOW_WIDTH*Y + X] = 0xff00ff;
		}
	}
}

static void
PresentBuffer(void)
{
	int SourceX = 0;
	int SourceY = 0;
	int DestinationX = 0;
	int DestinationY = 0;
	XPutImage(X11Display, X11Window, X11GraphicsContext, X11Image,
		SourceX, SourceY, DestinationX, DestinationY,
		WINDOW_WIDTH, WINDOW_HEIGHT);
}

static inline f64
GetTime(void)
{
	struct timespec Time = {0};
	clock_gettime(CLOCK_MONOTONIC, &Time);
	f64 Nanoseconds = Time.tv_nsec * 1e-9f;
	f64 Seconds = Time.tv_sec + Nanoseconds;
	f64 Milliseconds = Seconds * 1e-3f;
	return Milliseconds;
}

int
main(void)
{
	OpenWindow();

	f64 CurrentTimestamp = 0;
	f64 PreviousTimestamp = 0;
	f64 DeltaTime = 0;

	while (Running)
	{
		CurrentTimestamp = GetTime();
		DeltaTime = CurrentTimestamp - PreviousTimestamp;

		HandleInput();
		ClearBuffer();
		PresentBuffer();

		PreviousTimestamp = CurrentTime;
	}

	CloseWindow();
	return 0;
}
