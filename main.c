#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <time.h>

#include "base.h"
#include "glload.h"
#include "window.h"
#include "renderer.h"
#include "random.h"

#include "window.c"
#include "renderer.c"
#include "random.c"

u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[SAND] = 0xff00ffff,
	[WATER] = 0xffff0000,
};

static b32 Running = true;

enum { MAXIMUM_LOCATIONS_COUNT = 1<<4 };
static s32 LocationsCount;
static vector2s Locations[MAXIMUM_LOCATIONS_COUNT];
static vector2s PreviousLocation;

static cell_type Creating;

static inline b32
IsInWindowSpace(vector2s Location)
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
				else if (Event->keycode == XKeysymToKeycode(X11Display, XK_space))
				{
					memset(CellBuffer, 0, sizeof(CellBuffer));
				}
				break;
			}
			case ButtonPress:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				switch (Event->button)
				{
					case Button1: Creating = SAND; break;
					case Button3: Creating = WATER; break;
				}
				PreviousLocation.X = Event->x;
				PreviousLocation.Y = Event->y;
				break;
			}
			case ButtonRelease:
			{
				Creating = BLANK;
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				vector2s CellLocation = {0};
				CellLocation.X = Event->x;
				CellLocation.Y = Event->y;
				if (IsInWindowSpace(CellLocation))
				{
					Assert(LocationsCount < MAXIMUM_LOCATIONS_COUNT);
					Locations[LocationsCount++] = CellLocation;
					PreviousLocation = CellLocation;
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
	u32 Color = CellTypeColorTable[CellBuffer[CellY*X_CELL_COUNT + CellX]];
	s32 PixelY = CellY * CELL_SIZE;
	s32 PixelX = CellX * CELL_SIZE;

	Assert(PixelY >= 0);
	Assert(PixelX >= 0);
	Assert(PixelY < WINDOW_HEIGHT);
	Assert(PixelX < WINDOW_WIDTH);

	s32 StartY = Max(PixelY - HALF_CELL_SIZE, 0);
	s32 StartX = Max(PixelX - HALF_CELL_SIZE, 0);
	s32 EndY = Min(PixelY + HALF_CELL_SIZE, WINDOW_HEIGHT);
	s32 EndX = Min(PixelX + HALF_CELL_SIZE, WINDOW_WIDTH);
	for (s32 Y = StartY; Y < EndY; Y += 1)
	{
		for (s32 X = StartX; X < EndX; X += 1)
		{
			// NOTE(ariel) Invert y-texture coordinate for OpenGL.
			Framebuffer[(WINDOW_HEIGHT-Y-1)*WINDOW_WIDTH + X] = Color;
		}
	}
}

static void
TransitionWaterCell(s32 X, s32 Y)
{
	cell_type Cell3 = CellBuffer[(Y+0)*X_CELL_COUNT + (X-1)];
	cell_type Cell4 = CellBuffer[(Y+0)*X_CELL_COUNT + (X+1)];
	cell_type Cell5 = CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)];
	cell_type Cell6 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)];
	cell_type Cell7 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)];

	CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = BLANK;
	if (!Cell6)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)] = WATER;
	}
	else if (!Cell5)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)] = WATER;
	}
	else if (!Cell7)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)] = WATER;
	}
	else if (!Cell3)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X-1)] = WATER;
	}
	else if (!Cell4)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+1)] = WATER;
	}
	else
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
	}
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	cell_type Cell5 = CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)];
	cell_type Cell6 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)];
	cell_type Cell7 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)];

	CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = BLANK;
	if (!Cell6)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)] = SAND;
	}
	else if (Cell6 == WATER)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)] = SAND;
		TransitionWaterCell(X, Y);
	}
	else if (!Cell5)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)] = SAND;
	}
	else if (Cell5 == WATER)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)] = SAND;
		TransitionWaterCell(X, Y);
	}
	else if (!Cell7)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)] = SAND;
	}
	else if (Cell7 == WATER)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)] = SAND;
		TransitionWaterCell(X, Y);
	}
	else
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)] = SAND;
	}
}

int
main(void)
{
	OpenWindow();
	InitializeRenderer();

	u64 CurrentTimestamp = 0;
	u64 PreviousTimestamp = 0;
	u64 DeltaTime = 0;

	while (Running)
	{
		CurrentTimestamp = GetTime();

		HandleInput();

		// NOTE(ariel) Map new input in window coordinates to cell space.
		if (Creating)
		{
			for (s32 Index = 0; Index < LocationsCount - 1; Index += 1)
			{
				vector2s Location = Locations[Index];

				s32 CellY = Location.Y / CELL_SIZE;
				s32 CellX = Location.X / CELL_SIZE;

				Assert(CellY >= 0);
				Assert(CellX >= 0);
				Assert(CellY < Y_CELL_COUNT);
				Assert(CellX < X_CELL_COUNT);

				CellBuffer[CellY*X_CELL_COUNT + CellX] = Creating;
			}

			s32 PreviousLocationY = PreviousLocation.Y / CELL_SIZE;
			s32 PreviousLocationX = PreviousLocation.X / CELL_SIZE;

			if (Creating == SAND)
			{
				CellBuffer[PreviousLocationY*X_CELL_COUNT + PreviousLocationX] = Creating;
			}
			else if (Creating == WATER)
			{
				circle Cloud = {0};
				Cloud.Radius = 8;
				Cloud.Center.Y = PreviousLocationY;
				Cloud.Center.X = PreviousLocationX;
				for (s32 Y = -Cloud.Radius; Y < Cloud.Radius; Y += 1)
				{
					for (s32 X = -Cloud.Radius; X < Cloud.Radius; X += 1)
					{
						b32 PointIsInCircle = X*X + Y*Y <= Cloud.Radius * Cloud.Radius;
						b32 Chance = (RandomU32() & 7) == 7;
						cell_type *Cell = &CellBuffer[(Y+Cloud.Center.Y)*X_CELL_COUNT + (X+Cloud.Center.X)];
						cell_type OriginalType = *Cell;
						cell_type NewType = (cell_type)(Creating * PointIsInCircle * Chance);
						*Cell = !OriginalType ? NewType : OriginalType;
					}
				}
			}
		}

		// NOTE(ariel) Transition previous state.
		// FIXME(ariel) Cells that somehow manage to find their way to the
		// vertical edges do not transition ever.
		for (s32 Y = Y_CELL_COUNT - 2; Y > 1; Y -= 1)
		{
			for (s32 X = X_CELL_COUNT - 2; X > 1; X -= 1)
			{
				switch (CellBuffer[Y*X_CELL_COUNT + X])
				{
					case BLANK: break;
					case SAND: TransitionSandCell(X, Y); break;
					case WATER: TransitionWaterCell(X, Y); break;
					case CELL_TYPE_COUNT: break; // NOTE(ariel) Silence compiler warning.
				}
			}
		}

		for (s32 Y = 0; Y < Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = 0; X < X_CELL_COUNT; X += 1)
			{
				MapCellToPixels(Y, X);
			}
		}

		PresentBuffer();
		ClearBuffer();

		LocationsCount = 0;

		DeltaTime = CurrentTimestamp - PreviousTimestamp;
		PreviousTimestamp = CurrentTimestamp;
		SleepForNanoseconds(DeltaTime);
	}

	CloseWindow();
	return 0;
}
