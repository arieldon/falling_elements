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

static Display *X11Display;
static GLXContext X11GLContext;
static Window X11Window;
static Cursor InvisibleCursor;
static Atom X11DeleteWindowEvent;

typedef struct input input;
struct input
{
	s32 MousePositionX;
	s32 MousePositionY;
	s32 MouseDown;
	s32 PreviousMouseDown;
	b32 CursorIsInWindow;
};
static input Input;

static void OpenWindow(void);
static void CloseWindow(void);

static void HideCursor(void);
static void ShowCursor(void);

static void HandleInput(void);

#endif
