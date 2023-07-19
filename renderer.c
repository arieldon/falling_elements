static void
ClearBuffer(void)
{
	for (s32 Y = 0; Y < WINDOW_HEIGHT; Y += 1)
	{
		for (s32 X = 0; X < WINDOW_WIDTH; X += 1)
		{
			Framebuffer[WINDOW_WIDTH*Y + X] = 0xff00ff;
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
