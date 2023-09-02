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
static b32 Focused;

static void OpenWindow(void);
static void CloseWindow(void);

static void HandleInput(void);

#endif
