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
	long EventMask = KeyPressMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
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
		(char *)Framebuffer, WINDOW_WIDTH, WINDOW_HEIGHT,
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
	// NOTE(ariel) This also attempts to free the static buffer I gave it. |:^/
	// XDestroyImage(X11Image);
	XFreeGC(X11Display, X11GraphicsContext);
	XDestroyWindow(X11Display, X11Window);
	XCloseDisplay(X11Display);
}
