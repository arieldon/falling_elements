#include "immintrin.h"

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

u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[WOOD] = 0xff293136,
	[SAND] = 0xff00ccff,
	[WATER] = 0xffcc0000,
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
					case Button2: Creating = WOOD; break;
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
TransitionWaterCell(s32 X, s32 Y)
{
	cell *CellX = &CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)];
	cell_type Cell3 = CellBuffer[(Y+0)*X_CELL_COUNT + (X-1)].Type;
	cell_type Cell4 = CellBuffer[(Y+0)*X_CELL_COUNT + (X+1)].Type;
	cell_type Cell5 = CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)].Type;
	cell_type Cell6 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)].Type;
	cell_type Cell7 = CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)].Type;

	if (!Cell6)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (!Cell5)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (!Cell7)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (!Cell3)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X-1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (!Cell4)
	{
		CellBuffer[(Y+0)*X_CELL_COUNT + (X+1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	cell *CellX = &CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)];
	cell *Cell5 = &CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)];
	cell *Cell6 = &CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)];
	cell *Cell7 = &CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)];

	if (!Cell6->Type)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (Cell6->Type == WATER)
	{
		Swap(CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)], CellBuffer[(Y+1)*X_CELL_COUNT + (X+0)]);
		TransitionWaterCell(X, Y);
	}
	else if (!Cell5->Type)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (Cell5->Type == WATER)
	{
		Swap(CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)], CellBuffer[(Y+1)*X_CELL_COUNT + (X-1)]);
		TransitionWaterCell(X, Y);
	}
	else if (!Cell7->Type)
	{
		CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)] = *CellX;
		CellX->Type = BLANK;
		CellX->Color = 0;
	}
	else if (Cell7->Type == WATER)
	{
		Swap(CellBuffer[(Y+0)*X_CELL_COUNT + (X+0)], CellBuffer[(Y+1)*X_CELL_COUNT + (X+1)]);
		TransitionWaterCell(X, Y);
	}
}

int
main(void)
{
	random_sequence RandomSequence = SeedRandom();

	OpenWindow();
	renderer_context RendererContext = {0};
	InitializeRenderer(&RendererContext);

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

				CellBuffer[CellY*X_CELL_COUNT + CellX].Type = Creating;
				CellBuffer[CellY*X_CELL_COUNT + CellX].Color = CellTypeColorTable[Creating];
			}

			s32 PreviousLocationY = PreviousLocation.Y / CELL_SIZE;
			s32 PreviousLocationX = PreviousLocation.X / CELL_SIZE;

			if (Creating == SAND)
			{
				u32 Modifier = RandomU32InRange(&RandomSequence, 0x00, 0x33) << 8;
				CellBuffer[PreviousLocationY*X_CELL_COUNT + PreviousLocationX].Type = Creating;
				CellBuffer[PreviousLocationY*X_CELL_COUNT + PreviousLocationX].Color = CellTypeColorTable[Creating] + Modifier;
			}
			else if (Creating == WOOD)
			{
				u32 Modifier = RandomU32InRange(&RandomSequence, 0x00, 0x22);
				CellBuffer[PreviousLocationY*X_CELL_COUNT + PreviousLocationX].Type = Creating;
				CellBuffer[PreviousLocationY*X_CELL_COUNT + PreviousLocationX].Color = CellTypeColorTable[Creating] + Modifier;
			}
			else if (Creating == WATER)
			{
				for (s32 Y = -4; Y < 4; Y += 1)
				{
					for (s32 X = -4; X < 4; X += 1)
					{
						u32 CellY = Clamp(Y+PreviousLocationY, 0, Y_CELL_COUNT);
						u32 CellX = Clamp(X+PreviousLocationX, 0, X_CELL_COUNT);
						cell *Cell = &CellBuffer[X_CELL_COUNT*CellY + CellX];
						cell_type OriginalType = Cell->Type;
						if (!OriginalType)
						{
							u32 Modifier = RandomU32InRange(&RandomSequence, 0x00, 0x33) << 8;
							b32 Chance = RandomU32InRange(&RandomSequence, 0, 31) == 0;
							cell_type NewType = (cell_type)(Creating * Chance);
							Cell->Type = NewType;
							Cell->Color = CellTypeColorTable[NewType] + Modifier;
						}
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
				switch (CellBuffer[Y*X_CELL_COUNT + X].Type)
				{
					case BLANK: break;
					case WOOD: break; // NOTE(ariel) Wood cells remain stationary.
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
				cell_type Type = CellBuffer[X_CELL_COUNT*Y + X].Type;
				if (Type != BLANK)
				{
					s32 PixelY = Y * CELL_SIZE;
					s32 PixelX = X * CELL_SIZE;
					Quads[QuadsCount].Y = PixelY;
					Quads[QuadsCount].X = PixelX;
					Quads[QuadsCount].Color = CellBuffer[X_CELL_COUNT*Y + X].Color;
					QuadsCount += 1;
				}
			}
		}

		PresentBuffer();

		QuadsCount = 0;
		LocationsCount = 0;

		DeltaTime = CurrentTimestamp - PreviousTimestamp;
		PreviousTimestamp = CurrentTimestamp;
		SleepForNanoseconds(DeltaTime);
	}

	TerminateRenderer(RendererContext);
	CloseWindow();
	return 0;
}
