#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

#include "base.h"
#include "window.h"
#include "renderer.h"

#include "window.c"
#include "renderer.c"

enum { MAXIMUM_COORDINATES_COUNT = 1<<4 };
static s32 LocationsCount;
static Vector2s Locations[MAXIMUM_COORDINATES_COUNT];

static b32 Sanding;

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
				// XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				Sanding ^= Sanding;
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				Vector2s SandGrainLocation = {0};
				SandGrainLocation.X = Event->x;
				SandGrainLocation.Y = Event->y;
				Locations[LocationsCount++] = SandGrainLocation;
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

int
main(void)
{
	OpenWindow(Buffer);

	f64 CurrentTimestamp = 0;
	f64 PreviousTimestamp = 0;
	f64 DeltaTime = 0;

	while (Running)
	{
		CurrentTimestamp = GetTime();
		DeltaTime = CurrentTimestamp - PreviousTimestamp;

		HandleInput();

		for (s32 Index = 0; Index < LocationsCount; Index += 1)
		{
			Buffer[Locations[Index].Y*WINDOW_WIDTH + Locations[Index].X] = 0x0000ff;
		}
		PresentBuffer();

		LocationsCount = 0;
		PreviousTimestamp = CurrentTime;
	}

	CloseWindow();
	return 0;
}
