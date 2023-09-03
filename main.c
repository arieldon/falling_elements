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
IsInWindowSpace(s32 X, s32 Y)
{
	b32 XMin = X >= 0;
	b32 YMin = Y >= 0;
	b32 XMax = X < WINDOW_WIDTH;
	b32 YMax = Y < WINDOW_HEIGHT;
	return XMin & YMin & XMax & YMax;
}

static inline b32
ShouldCreateCell(void)
{
	b32 InWindowSpace = IsInWindowSpace(Input.MousePositionX, Input.MousePositionY);
	b32 Result = Input.MouseDown & InWindowSpace & !MenuContext.MenuIsHot;
	return Result;
}

static inline void
SetBrushPixel(s32 X, s32 Y)
{
	Quads[QuadsCount].X = X;
	Quads[QuadsCount].Y = Y;
	Quads[QuadsCount].Width = 1;
	Quads[QuadsCount].Height = 1;
	Quads[QuadsCount].Color = 0xffffffff;
	Quads[QuadsCount].TextureID = MENU_ICON_BLANK;
	QuadsCount += 1;
}

static void
DrawBrush(void)
{
	if (!MenuContext.MenuIsHot && Input.CursorIsInWindow)
	{
		int Cx = Input.MousePositionX;
		int Cy = Input.MousePositionY;
		int R = 32;

		// NOTE(casey): Loop that draws the circle
		{
			int R2 = R+R;

			int X = R;
			int Y = 0;
			int dY = -2;
			int dX = R2+R2 - 4;
			int D = R2 - 1;

			while(Y <= X)
			{
				SetBrushPixel(Cx - X, Cy - Y);
				SetBrushPixel(Cx + X, Cy - Y);
				SetBrushPixel(Cx - X, Cy + Y);
				SetBrushPixel(Cx + X, Cy + Y);
				SetBrushPixel(Cx - Y, Cy - X);
				SetBrushPixel(Cx + Y, Cy - X);
				SetBrushPixel(Cx - Y, Cy + X);
				SetBrushPixel(Cx + Y, Cy + X);

				D += dY;
				dY -= 4;
				++Y;

				int Mask = (D >> 31);
				D += dX & Mask;
				dX -= 4 & Mask;
				X += Mask;
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

	BoundCells();

	while (Running)
	{
		CurrentTimestamp = GetTime();

		HandleInput();

		{
			BeginMenu();

			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[SAND]))
			{
				Creating = SAND;
			}
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[WATER]))
			{
				Creating = WATER;
			}
			if (MenuButton(MENU_ICON_BLANK, 0xff00bb00))
			{
				Creating = GAS;
			}
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[WOOD]))
			{
				Creating = WOOD;
			}
			if (MenuButton(MENU_ICON_BLANK, CellTypeColorTable[FIRE]))
			{
				Creating = FIRE;
			}
			if (MenuButton(MENU_ICON_BLANK, 0xff000000))
			{
				Creating = BLANK;
			}
			if (MenuButton(MENU_ICON_CLEAR, 0xff000000))
			{
				ShouldClearScreen = true;
			}
			if (Playing ? MenuButton(MENU_ICON_PAUSE, 0xff333333) : MenuButton(MENU_ICON_PLAY, 0xff333333))
			{
				Playing ^= 1;
			}

			EndMenu();
		}

		// NOTE(ariel) Map new input in window coordinates to cell space.
		if (ShouldCreateCell())
		{
			s32 LocationY = Input.MousePositionY / CELL_SIZE + CELL_START;
			s32 LocationX = Input.MousePositionX / CELL_SIZE + CELL_START;
			SpawnCells(LocationX, LocationY);
		}

		if (ShouldClearScreen)
		{
			memset(CellBuffer, BLANK, sizeof(CellBuffer));
			BoundCells();
			ShouldClearScreen = false;
		}

		if (FrameCount & 1)
		{
			for (s32 Y = Playing*Y_CELL_COUNT; Y >= CELL_START; Y -= 1)
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
			for (s32 Y = Playing*Y_CELL_COUNT; Y >= CELL_START; Y -= 1)
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
					Quads[QuadsCount].Color = CellTypeColorTable[Cell(X, Y).Type] + (Cell(X, Y).ColorModification<<8);
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

		DrawBrush();

		for (s32 Y = CELL_START; Y <= Y_CELL_COUNT; Y += 1)
		{
			for (s32 X = CELL_START; X <= X_CELL_COUNT; X += 1)
			{
				Cell(X, Y).Updated = false;
			}
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
