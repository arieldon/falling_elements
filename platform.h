#ifndef WINDOW_H
#define WINDOW_H

enum
{
	WINDOW_WIDTH = 1280,
	WINDOW_HEIGHT = 720,
	WINDOW_BORDER_WIDTH = 0,
};
StaticAssert(WINDOW_WIDTH > 0);
StaticAssert(WINDOW_HEIGHT > 0);

typedef struct input input;
struct input
{
	s32 MousePositionX;
	s32 MousePositionY;
	s32 MouseDown;
	b32 CursorIsInWindow;
};
static input Input;

static void PlatformOpenWindow(void);
static void PlatformCloseWindow(void);

static void PlatformSwapBuffers(void);

static void PlatformShowCursor(b32 ShouldShowCursor);

static void PlatformHandleInput(void);

static inline u64 PlatformGetTime(void);
static inline void PlatformSleep(u64 DeltaTime);

#endif
