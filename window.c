typedef GLXContext (* glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, b32, int *);
typedef void (* glXSwapIntervalEXTProc)(Display *, GLXDrawable, int);
typedef int (* glXSwapIntervalSGIProc)(int);
typedef int (* glXSwapIntervalMESAProc)(unsigned int);

static void
OpenWindow(void)
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
	XStoreName(X11Display, X11Window, "Sand");

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
		Color.red = Color.red = Color.blue = 0;

		Pixmap PixmapID = XCreatePixmap(X11Display, X11DefaultRootWindow, 1, 1, 1);
		Assert(PixmapID);

		InvisibleCursor = XCreatePixmapCursor(X11Display, PixmapID, PixmapID, &Color, &Color, 0, 0);
		XFreePixmap(X11Display, PixmapID);

		XDefineCursor(X11Display, X11Window, InvisibleCursor);
	}

	XFree(ViableFramebuffers);
	XFlush(X11Display);
}

static void
CloseWindow(void)
{
	glXDestroyContext(X11Display, X11GLContext);
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
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				MenuInputMouseButtonPress(Event->x, Event->y);
				break;
			}
			case ButtonRelease:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				MenuInputMouseButtonRelease(Event->x, Event->y);
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				MenuInputMouseMove(Event->x, Event->y);
				break;
			}
			case EnterNotify:
			{
				Focused = true;
				break;
			}
			case LeaveNotify:
			{
				Focused = false;
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
