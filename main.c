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

static void
MapCellToPixels(s32 CellY, s32 CellX)
{
	s32 PixelY = CellY * 5;
	s32 PixelX = CellX * 5;

	Assert(PixelY >= 0);
	Assert(PixelX >= 0);
	Assert(PixelY < WINDOW_HEIGHT);
	Assert(PixelX < WINDOW_WIDTH);

	s32 StartY = Max(PixelY - 2, 0);
	s32 StartX = Max(PixelX - 2, 0);
	s32 EndY = Min(PixelY + 2, WINDOW_HEIGHT);
	s32 EndX = Min(PixelX + 2, WINDOW_WIDTH);
	for (s32 Y = StartY; Y < EndY; Y += 1)
	{
		for (s32 X = StartX; X < EndX; X += 1)
		{
			Framebuffer[Y*WINDOW_WIDTH + X] = YELLOW;
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

		s32 InactiveSandBufferIndex = ActiveSandBufferIndex ^ 1;

		// NOTE(ariel) Transition previous state.
		{
			// NOTE(ariel) Persist state of bottom row of cells across frames.
			for (s32 X = 0; X < X_CELL_COUNT; X += 1)
			{
				u32 PreviousCellState = SandBuffers[InactiveSandBufferIndex][(Y_CELL_COUNT-1)*X_CELL_COUNT + X];
				SandBuffers[ActiveSandBufferIndex][(Y_CELL_COUNT-1)*X_CELL_COUNT + X] = PreviousCellState;
			}

			// NOTE(ariel) Model gravity.
			for (s32 Y = 0; Y < Y_CELL_COUNT - 1; Y += 1)
			{
				for (s32 X = 0; X < X_CELL_COUNT; X += 1)
				{
					u32 PreviousCellState = SandBuffers[InactiveSandBufferIndex][Y*X_CELL_COUNT + X];
					u32 PreviousBottomNeighborState = SandBuffers[InactiveSandBufferIndex][(Y+1)*X_CELL_COUNT + X];
					if (PreviousCellState && PreviousBottomNeighborState)
					{
						SandBuffers[ActiveSandBufferIndex][(Y+0)*X_CELL_COUNT + X] = YELLOW;
					}
					else if (PreviousCellState)
					{
						SandBuffers[ActiveSandBufferIndex][(Y+1)*X_CELL_COUNT + X] = YELLOW;
					}
				}
			}
		}

		// NOTE(ariel) Map new input in window coordinates to cell space.
		if (Sanding)
		{
			for (s32 Index = 0; Index < LocationsCount - 1; Index += 1)
			{
				Vector2s Location = Locations[Index];

				s32 CellY = Location.Y / 5;
				s32 CellX = Location.X / 5;

				Assert(CellY >= 0);
				Assert(CellX >= 0);
				Assert(CellY < Y_CELL_COUNT);
				Assert(CellX < X_CELL_COUNT);

				SandBuffers[ActiveSandBufferIndex][CellY*X_CELL_COUNT + CellX] = YELLOW;
			}

			s32 PreviousLocationY = PreviousLocation.Y / 5;
			s32 PreviousLocationX = PreviousLocation.X / 5;
			SandBuffers[ActiveSandBufferIndex][PreviousLocationY*X_CELL_COUNT + PreviousLocationX] = YELLOW;
		}

		for (s32 Y = 0; Y < Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = 0; X < X_CELL_COUNT; X += 1)
			{
				if (SandBuffers[ActiveSandBufferIndex][Y*X_CELL_COUNT + X])
				{
					MapCellToPixels(Y, X);
				}
			}
		}

		PresentBuffer();
		memset(Framebuffer, 0xffff00, WINDOW_HEIGHT*WINDOW_WIDTH*sizeof(u32));
		memset(SandBuffers[InactiveSandBufferIndex], 0, Y_CELL_COUNT*X_CELL_COUNT*sizeof(u32));

		LocationsCount = 0;
		ActiveSandBufferIndex ^= 1;
		PreviousTimestamp = CurrentTime;
	}

	CloseWindow();
	return 0;
}
