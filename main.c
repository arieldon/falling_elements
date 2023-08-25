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

static b32 Running = true;
static b32 Playing = true;
static b32 ShouldClearScreen;

static cell_type Creating = SAND;

#include "window.c"
#include "renderer.c"
#include "automata.c"
#include "menu.c"


static inline b32
IsInWindowSpace(vector2s Location)
{
	b32 XMin = Location.X >= 0;
	b32 YMin = Location.Y >= 0;
	b32 XMax = Location.X < WINDOW_WIDTH;
	b32 YMax = Location.Y < WINDOW_HEIGHT;
	return XMin & YMin & XMax & YMax;
}

static inline b32
ShouldCreateCell(void)
{
	b32 MenuNotHot = !MouseOverTarget(MenuContext.EntireMenu);
	b32 InWindowSpace = IsInWindowSpace(MenuContext.MousePosition);
	b32 Result = MenuNotHot & InWindowSpace & MenuContext.MouseDown;
	return Result;
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

	BoundCells();

	while (Running)
	{
		CurrentTimestamp = GetTime();

		HandleInput();

		{
			BeginMenu();

			// TODO(ariel)
			// - overlay checkmark or something on selected cell type
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[SAND], StringLiteral("Sand")))
			{
				Creating = SAND;
			}
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[WATER], StringLiteral("Water")))
			{
				Creating = WATER;
			}
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[WOOD], StringLiteral("Wood")))
			{
				Creating = WOOD;
			}
			if (MenuButton(MENU_ICON_BLANK, 0xff000000, StringLiteral("Blank")))
			{
				Creating = BLANK;
			}
			if (MenuButton(MENU_ICON_CLEAR, 0xff000000, StringLiteral("Clear")))
			{
				ShouldClearScreen = true;
			}
			if (Playing ? MenuButton(MENU_ICON_PAUSE, 0xff333333, StringLiteral("Pause")) : MenuButton(MENU_ICON_PLAY, 0xff333333, StringLiteral("Play")))
			{
				Playing ^= 1;
			}

			EndMenu();
		}

		// NOTE(ariel) Map new input in window coordinates to cell space.
		if (ShouldCreateCell())
		{
			s32 LocationY = MenuContext.MousePositionY / CELL_SIZE + CELL_START;
			s32 LocationX = MenuContext.MousePositionX / CELL_SIZE + CELL_START;
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
			else if (Creating == BLANK)
			{
				Cell(LocationX, LocationY).Type = Creating;
				Cell(LocationX, LocationY).Color = CellTypeColorTable[Creating];
			}
		}

		if (ShouldClearScreen)
		{
			memset(CellBuffer, BLANK, sizeof(CellBuffer));
			BoundCells();
			ShouldClearScreen = false;
		}

		if (Playing)
		{
			if (FrameCount & 1)
			{
				for (s32 Y = Y_CELL_COUNT; Y >= CELL_START; Y -= 1)
				{
					s32 Y0 = Y;
					s32 Y1 = Y-1;
					for (s32 ReverseX = X_CELL_COUNT; ReverseX >= CELL_START; ReverseX -= 1)
					{
						TransitionCell(ReverseX, Y0);
					}
					for (s32 ForwardX = CELL_START; ForwardX <= X_CELL_COUNT; ForwardX += 1)
					{
						TransitionCell(ForwardX, Y1);
					}
				}
			}
			else
			{
				for (s32 Y = Y_CELL_COUNT; Y >= CELL_START; Y -= 1)
				{
					s32 Y0 = Y;
					s32 Y1 = Y-1;
					for (s32 ForwardX = CELL_START; ForwardX <= X_CELL_COUNT; ForwardX += 1)
					{
						TransitionCell(ForwardX, Y0);
					}
					for (s32 ReverseX = X_CELL_COUNT; ReverseX >= CELL_START; ReverseX -= 1)
					{
						TransitionCell(ReverseX, Y1);
					}
				}
			}
		}

		for (s32 Y = CELL_START; Y <= Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = CELL_START; X <= X_CELL_COUNT; X += 1)
			{
				if (Cell(X, Y).Type != BLANK)
				{
					s32 PixelY = (Y-CELL_START) * CELL_SIZE;
					s32 PixelX = (X-CELL_START) * CELL_SIZE;
					Quads[QuadsCount].Y = PixelY;
					Quads[QuadsCount].X = PixelX;
					Quads[QuadsCount].Width = CELL_SIZE;
					Quads[QuadsCount].Height = CELL_SIZE;
					Quads[QuadsCount].Color = Cell(X, Y).Color;
					Quads[QuadsCount].TextureID = MENU_ICON_BLANK;
					QuadsCount += 1;
				}
			}
		}

		for (s32 Index = 0; Index < MenuContext.CommandCount; Index += 1)
		{
			Quads[QuadsCount] = MenuContext.Commands[Index];
			QuadsCount += 1;
		}

		PresentBuffer();

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
