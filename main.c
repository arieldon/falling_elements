#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

#include "base.h"
#include "window.h"
#include "renderer.h"

#include "window.c"
#include "renderer.c"

u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x000000,
	[SAND] = 0xffff00,
	[WATER] = 0x0000ff,
};

enum { MAXIMUM_LOCATIONS_COUNT = 1<<4 };
static s32 LocationsCount;
static Vector2s Locations[MAXIMUM_LOCATIONS_COUNT];
static Vector2s PreviousLocation;

static cell_type Creating;

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
				else if (Event->keycode == XKeysymToKeycode(X11Display, XK_space))
				{
					memset(&CellBuffers[InactiveCellBufferIndex], 0, sizeof(CellBuffers[InactiveCellBufferIndex]));
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
				Vector2s CellLocation = {0};
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
	u32 Color = CellTypeColorTable[CellBuffers[ActiveCellBufferIndex][CellY*X_CELL_COUNT + CellX]];
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
			Framebuffer[Y*WINDOW_WIDTH + X] = Color;
		}
	}
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	cell_type CellX = CellBuffers[InactiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)];
	cell_type Cell5 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X-1)];
	cell_type Cell6 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+0)];
	cell_type Cell7 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+1)];

	if (!Cell6)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+0)] = SAND;
	}
	else if (Cell6 == WATER)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+0)] = SAND;
	}
	else if (!Cell5)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X-1)] = SAND;
	}
	else if (Cell5 == WATER)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X-1)] = SAND;
	}
	else if (!Cell7)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+1)] = SAND;
	}
	else if (Cell7 == WATER)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+1)] = SAND;
	}
	else
	{
		CellBuffers[ActiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)] = SAND;
	}
}

static void
TransitionWaterCell(s32 X, s32 Y)
{
	cell_type CellX = CellBuffers[InactiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)];
	cell_type Cell3 = CellBuffers[InactiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X-1)];
	cell_type Cell4 = CellBuffers[InactiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+1)];
	cell_type Cell5 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X-1)];
	cell_type Cell6 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+0)];
	cell_type Cell7 = CellBuffers[InactiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+1)];

	if (!Cell6)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+0)] = WATER;
	}
	else if (!Cell5)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X-1)] = WATER;
	}
	else if (!Cell7)
	{
		CellBuffers[ActiveCellBufferIndex][(Y+1)*X_CELL_COUNT + (X+1)] = WATER;
	}
	else
	{
		CellBuffers[ActiveCellBufferIndex][(Y+0)*X_CELL_COUNT + (X+0)] = WATER;
	}
}

int
main(void)
{
	OpenWindow();

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
				Vector2s Location = Locations[Index];

				s32 CellY = Location.Y / 5;
				s32 CellX = Location.X / 5;

				Assert(CellY >= 0);
				Assert(CellX >= 0);
				Assert(CellY < Y_CELL_COUNT);
				Assert(CellX < X_CELL_COUNT);

				CellBuffers[ActiveCellBufferIndex][CellY*X_CELL_COUNT + CellX] = Creating;
			}

			s32 PreviousLocationY = PreviousLocation.Y / 5;
			s32 PreviousLocationX = PreviousLocation.X / 5;
			switch (Creating)
			{
				case SAND:
				{
					if (!CellBuffers[InactiveCellBufferIndex][PreviousLocationY*X_CELL_COUNT + PreviousLocationX])
					{
						CellBuffers[ActiveCellBufferIndex][PreviousLocationY*X_CELL_COUNT + PreviousLocationX] = Creating;
					}
					break;
				}
				case WATER:
				{
					// TODO(ariel) Create "cloud" of water cells. Define the cloud as a
					// circle, i.e. radius and center, and fill cells within radius
					// randomly.
					CellBuffers[ActiveCellBufferIndex][PreviousLocationY*X_CELL_COUNT + PreviousLocationX] = Creating;
					CellBuffers[ActiveCellBufferIndex][PreviousLocationY*X_CELL_COUNT + (PreviousLocationX-1)] = Creating;
					CellBuffers[ActiveCellBufferIndex][PreviousLocationY*X_CELL_COUNT + (PreviousLocationX+1)] = Creating;
					break;
				}
			}
		}

		// NOTE(ariel) Transition previous state.
		{
			// NOTE(ariel) Persist state of bottom row of cells across frames.
			for (s32 X = 0; X < X_CELL_COUNT; X += 1)
			{
				cell_type PreviousCellState = CellBuffers[InactiveCellBufferIndex][(Y_CELL_COUNT-1)*X_CELL_COUNT + X];
				CellBuffers[ActiveCellBufferIndex][(Y_CELL_COUNT-1)*X_CELL_COUNT + X] = PreviousCellState;
			}

			for (s32 Y = Y_CELL_COUNT - 2; Y > 0; Y -= 1)
			{
				for (s32 X = 1; X < X_CELL_COUNT - 1; X += 1)
				{
					switch (CellBuffers[InactiveCellBufferIndex][Y*X_CELL_COUNT + X])
					{
						case BLANK: break;
						case SAND: TransitionSandCell(X, Y); break;
						case WATER: TransitionWaterCell(X, Y); break;
						case CELL_TYPE_COUNT: break; // NOTE(ariel) Silence compiler warning.
					}
				}
			}
		}

		for (s32 Y = 0; Y < Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = 0; X < X_CELL_COUNT; X += 1)
			{
				if (CellBuffers[ActiveCellBufferIndex][Y*X_CELL_COUNT + X])
				{
					MapCellToPixels(Y, X);
				}
			}
		}

		PresentBuffer();
		memset(Framebuffer, 0, WINDOW_HEIGHT*WINDOW_WIDTH*sizeof(u32));
		memset(CellBuffers[InactiveCellBufferIndex], 0, Y_CELL_COUNT*X_CELL_COUNT*sizeof(cell_type));

		LocationsCount = 0;
		ActiveCellBufferIndex ^= 1;
		InactiveCellBufferIndex = ActiveCellBufferIndex ^ 1;

		DeltaTime = CurrentTimestamp - PreviousTimestamp;
		PreviousTimestamp = CurrentTimestamp;
		SleepForNanoseconds(DeltaTime);
	}

	CloseWindow();
	return 0;
}
