#include "immintrin.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <time.h>

#include "base.h"
#include "string.h"
#include "random.h"
#include "glload.h"
#include "window.h"
#include "renderer.h"
#include "automata.h"
#include "menu.h"

#include "window.c"
#include "renderer.c"
#include "automata.c"
#include "menu.c"

static b32 Running = true;
static b32 ShouldClearScreen;

static cell_type Creating = SAND;

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
					ShouldClearScreen = true;
				}
				break;
			}
			case ButtonPress:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				MenuInputMouseButtonPress(Event->x, Event->y);
				break;
			}
			case ButtonRelease:
			{
				XButtonEvent *Event = (XButtonEvent *)&GeneralEvent;
				MenuInputMouseButtonRelease(Event->x, Event->y);
				break;
			}
			case MotionNotify:
			{
				XPointerMovedEvent *Event = (XPointerMovedEvent *)&GeneralEvent;
				MenuInputMouseMove(Event->x, Event->y);
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
	SeedRandom();

	OpenWindow();
	renderer_context RendererContext = {0};
	InitializeRenderer(&RendererContext);

	u64 FrameCount = 0;
	u64 CurrentTimestamp = 0;
	u64 PreviousTimestamp = 0;
	u64 DeltaTime = 0;

	CreateBoundary();

	while (Running)
	{
		CurrentTimestamp = GetTime();

		HandleInput();

		{
			BeginMenu();

			// TODO(ariel)
			// - layout buttons automagically
			// - button per user creatable cell type
			// - button for eraser
			// - button to clear screen
			// - button to play/pause
			// - hide menu by default and pop it out when cursor hovers near it
			if (MenuButton(CellTypeColorTable[SAND], StringLiteral("Sand")))
			{
				Creating = SAND;
			}
			if (MenuButton(CellTypeColorTable[WATER], StringLiteral("Water")))
			{
				Creating = WATER;
			}
			if (MenuButton(CellTypeColorTable[WOOD], StringLiteral("Wood")))
			{
				Creating = WOOD;
			}

			EndMenu();
		}

		// NOTE(ariel) Map new input in window coordinates to cell space.
		// FIXME(ariel) Check bounds of mouse pointer location.
		if (MenuContext.MouseDown && !MouseOverTarget(MenuContext.EntireMenu) && IsInWindowSpace(MenuContext.MousePosition))
		{
			s32 LocationY = MenuContext.MousePositionY / CELL_SIZE;
			s32 LocationX = MenuContext.MousePositionX / CELL_SIZE;
			if (Cell(LocationX, LocationY).Type == BLANK)
			{
				if (Creating == SAND)
				{
					u32 Modifier = RandomU32InRange(0x00, 0x33) << 8;
					Cell(LocationX, LocationY).Type = Creating;
					Cell(LocationX, LocationY).Color = CellTypeColorTable[Creating] + Modifier;
				}
				else if (Creating == WOOD)
				{
					u32 Modifier = RandomU32InRange(0x00, 0x22);
					Cell(LocationX, LocationY).Type = Creating;
					Cell(LocationX, LocationY).Color = CellTypeColorTable[Creating] + Modifier;
				}
				else if (Creating == WATER)
				{
					for (s32 Y = -4; Y < 4; Y += 1)
					{
						for (s32 X = -4; X < 4; X += 1)
						{
							u32 CellY = Clamp(Y+LocationY, 0, Y_CELL_COUNT);
							u32 CellX = Clamp(X+LocationX, 0, X_CELL_COUNT);
							cell_type OriginalType = Cell(CellX, CellY).Type;
							if (!OriginalType)
							{
								u32 Modifier = RandomU32InRange(0x00, 0x33) << 8;
								b32 Chance = RandomU32InRange(0, 31) == 0;
								cell_type NewType = (cell_type)(Creating * Chance);
								Cell(CellX, CellY).Type = NewType;
								Cell(CellX, CellY).Color = CellTypeColorTable[NewType] + Modifier;
							}
						}
					}
				}
			}
		}

		if (ShouldClearScreen)
		{
			memset(CellBuffer, 0, sizeof(CellBuffer));
			CreateBoundary();
			ShouldClearScreen = false;
		}

		if (FrameCount & 1)
		{
			for (s32 Y = Y_CELL_COUNT-1; Y > CELL_START; Y -= 1)
			{
				s32 Y0 = Y;
				s32 Y1 = Y-1;
				for (s32 ReverseX = X_CELL_COUNT-1; ReverseX >= CELL_START; ReverseX -= 1)
				{
					TransitionCell(ReverseX, Y0);
				}
				for (s32 ForwardX = CELL_START; ForwardX < X_CELL_COUNT; ForwardX += 1)
				{
					TransitionCell(ForwardX, Y1);
				}
			}
		}
		else
		{
			for (s32 Y = Y_CELL_COUNT-1; Y > CELL_START; Y -= 1)
			{
				s32 Y0 = Y;
				s32 Y1 = Y-1;
				for (s32 ForwardX = CELL_START; ForwardX < X_CELL_COUNT; ForwardX += 1)
				{
					TransitionCell(ForwardX, Y0);
				}
				for (s32 ReverseX = X_CELL_COUNT-1; ReverseX >= CELL_START; ReverseX -= 1)
				{
					TransitionCell(ReverseX, Y1);
				}
			}
		}

		for (s32 Y = CELL_START; Y < Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = CELL_START; X < X_CELL_COUNT; X += 1)
			{
				if (Cell(X, Y).Type != BLANK)
				{
					s32 PixelY = Y * CELL_SIZE;
					s32 PixelX = X * CELL_SIZE;
					Quads[QuadsCount].Y = PixelY;
					Quads[QuadsCount].X = PixelX;
					Quads[QuadsCount].Color = Cell(X, Y).Color;
					QuadsCount += 1;
				}
			}
		}

		for (s32 Index = 0; Index < MenuContext.CommandCount; Index += 1)
		{
			MenuQuads[MenuQuadsCount].Y = MenuContext.Commands[Index].Target.Y;
			MenuQuads[MenuQuadsCount].X = MenuContext.Commands[Index].Target.X;
			MenuQuads[MenuQuadsCount].Color = MenuContext.Commands[Index].Color;
			MenuQuadsCount += 1;
		}

		PresentBuffer(RendererContext);

		MenuQuadsCount = 0;
		QuadsCount = 0;
		FrameCount += 1;

		DeltaTime = CurrentTimestamp - PreviousTimestamp;
		PreviousTimestamp = CurrentTimestamp;
		SleepForNanoseconds(DeltaTime);
	}

	TerminateRenderer(RendererContext);
	CloseWindow();
	return 0;
}
