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

static inline u64
GetTime(void)
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
SleepForNanoseconds(u64 DeltaTimeNS)
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
