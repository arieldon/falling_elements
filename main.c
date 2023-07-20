#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

#include "base.h"
#include "window.h"
#include "renderer.h"

#include "window.c"
#include "renderer.c"

enum { YELLOW = 0xffff00 };

enum { MAXIMUM_COORDINATES_COUNT = 1<<4 };
static s32 LocationsCount;
static Vector2s Locations[MAXIMUM_COORDINATES_COUNT];
static Vector2s PreviousLocation;

static b32 Sanding;

static inline b32
IsInWindowSpace(Vector2s Location)
{
	b32 XMin = Location.X >= 0;
	b32 YMin = Location.Y >= 0;
	b32 XMax = Location.X < WINDOW_WIDTH;
	b32 YMax = Location.Y < WINDOW_HEIGHT;
	return XMin & YMin & XMax & YMax;
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
				Sanding = 1;
				PreviousLocation.X = Event->x;
				PreviousLocation.Y = Event->y;
				break;
			}
			case ButtonRelease:
			{
				Sanding = 0;
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				Vector2s SandGrainLocation = {0};
				SandGrainLocation.X = Event->x;
				SandGrainLocation.Y = Event->y;
				if (IsInWindowSpace(SandGrainLocation))
				{
					Locations[LocationsCount++] = SandGrainLocation;
					PreviousLocation = SandGrainLocation;
				}
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
	OpenWindow();

	f64 CurrentTimestamp = 0;
	f64 PreviousTimestamp = 0;
	f64 DeltaTime = 0;

	while (Running)
	{
		CurrentTimestamp = GetTime();
		DeltaTime = CurrentTimestamp - PreviousTimestamp;

		HandleInput();

		// NOTE(ariel) Transition previous state.
		u32 OffSandBufferIndex = SandBufferIndex ^ 1;
		for (s32 Y = 0; Y < WINDOW_HEIGHT - 1; Y += 1)
		{
			for (s32 X = 0; X < WINDOW_WIDTH; X += 1)
			{
				if (SandBuffers[OffSandBufferIndex][Y*WINDOW_WIDTH + X] == YELLOW)
				{
					SandBuffers[SandBufferIndex][(Y+1)*WINDOW_WIDTH + X] = YELLOW;
				}
			}
		}

		memset(SandBuffers[OffSandBufferIndex], 0, sizeof(u32) * WINDOW_WIDTH * WINDOW_HEIGHT);

		// NOTE(ariel) Add new states from input.
		if (Sanding)
		{
			for (s32 Index = 0; Index < LocationsCount - 1; Index += 1)
			{
				SandBuffers[SandBufferIndex][Locations[Index].Y*WINDOW_WIDTH + Locations[Index].X] = YELLOW;
			}
			SandBuffers[SandBufferIndex][PreviousLocation.Y*WINDOW_WIDTH + PreviousLocation.X] = YELLOW;
		}

		memcpy(Framebuffer, SandBuffers[SandBufferIndex], sizeof(u32) * WINDOW_WIDTH * WINDOW_HEIGHT);
		PresentBuffer();

		SandBufferIndex ^= 1;
		LocationsCount = 0;
		PreviousTimestamp = CurrentTime;
	}

	CloseWindow();
	return 0;
}
