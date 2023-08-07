#ifndef WINDOW_H
#define WINDOW_H

enum
{
	WINDOW_WIDTH = 1920,
	WINDOW_HEIGHT = 1080,
	WINDOW_BORDER_WIDTH = 0,
};
StaticAssert(WINDOW_WIDTH > 0);
StaticAssert(WINDOW_HEIGHT > 0);

static u32 Framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT];

static Display *X11Display;
static GLXContext X11GLContext;
static Window X11Window;
static Atom X11DeleteWindowEvent;

static void OpenWindow(void);
static void CloseWindow(void);

#endif
