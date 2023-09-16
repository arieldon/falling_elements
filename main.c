#include "immintrin.h"

#include "base.h"
#include "string.h"
#include "random.h"
#include "platform.h"
#include "glload.h"
#include "renderer.h"
#include "automata.h"
#include "menu.h"

static b32 Running = true;
static b32 Playing = true;
static b32 ShouldClearScreen;

static cell_type Creating = SAND;

#if defined(__linux__)
#include "platform_linux.c"
#elif defined(_WIN64)
#include "platform_windows.c"
#else
#error `platform.h` is not implemented on this platform.
#endif
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
ShouldSpawnCells(void)
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
	// NOTE(ariel) I stole this code from Casey Muratori's Computer Enhance
	// lecture [0] where he explains his implementation of a DDA (Digital
	// Differential Analyzer) circle outline algorithm.
	//
	// It's far from the most efficient method to draw a circle on modern
	// hardware, especially my use of SetBrushPixel() below, but it's super
	// simple. It doesn't even use multiplication explicitly! Plus, I don't need
	// to link and include `math.h` this way or implement sin() and cos() with
	// polynomial approximations or some other method.
	//
	// [0] https://www.computerenhance.com/p/efficient-dda-circle-outlines
	if (!MenuContext.MenuIsHot && Input.CursorIsInWindow)
	{
		s32 CenterX = Input.MousePositionX;
		s32 CenterY = Input.MousePositionY;

		s32 Radius = 32;
		s32 Diameter = Radius + Radius;

		// NOTE(ariel) Start drawing circle's outline from x-axis in first
		// quadrant: x = (Radius, 0).
		//       |
		//       |
		// ------+----x-- Let Radius = 5 in this case.
		//       |
		//       |
		s32 X = Radius;
		s32 Y = 0;

		s32 dY = -2;
		s32 dX = Diameter+Diameter - 4;
		s32 D = Diameter - 1;

		// NOTE(ariel) Use a signed distance field specified by the implicit
		// definition of a circle f(x, y) = x*x + y*y - r*r = 0, though we don't
		// use this formula directly because we simplify it further for this
		// specific case. An implicit definition serves this problem well because
		// the plotting _starts_ on the circle at (Radius, 0) by default. With each
		// iteration of the loop, the algorithm then chooses which pixel to plot
		// next.
		//
		// We essentially choose between two points A and B in each iteration,
		// where A=(Radius-1, Y+1) and B=(Radius, Y+1). We choose the point closer
		// to the center of the circle, i.e. |f(A)| < |f(B)| choose A, otherwise B.
		//
		// If we plug A and B into the implicit circle equation f above and push
		// all the variables onto one side to compare against zero, we find this
		// equation: d(x, y) = -2x*x + 2x - 2y*y + c ?< 0, where c=2r*r - 1.
		//
		// Next we use a digital differential analyzer to find the following
		// equation: e(x, y) = d(x, y+1) - d(x, y) = -4y - 2. This equation
		// indicates change per step -- they're derivatives, and it's where the
		// terms for `dY` originate. We then apply the same technique again as
		// follows: e(x, y+1) - e(x, y) = -4.
		//
		// We follow the same procedure for d(x-1, y) - d(x, y) to find the change
		// in `X` each iteration of the loop.
		while (Y <= X)
		{
			// NOTE(ariel) Exploit the circle's perfect symmetry to plot all octant
			// whilst only explicitly computing coordinates for the first, where the
			// first octant is the half of the first quadrant closer to the x-axis.
			// This allows us to increment `Y` in each iteration of the loop since we
			// stop before the relationship between X and Y essentially flips,
			// becoming A=(X-1,Radius-1) and B=(X-1, Radius).
			SetBrushPixel(CenterX - X, CenterY - Y);
			SetBrushPixel(CenterX + X, CenterY - Y);
			SetBrushPixel(CenterX - X, CenterY + Y);
			SetBrushPixel(CenterX + X, CenterY + Y);
			SetBrushPixel(CenterX - Y, CenterY - X);
			SetBrushPixel(CenterX + Y, CenterY - X);
			SetBrushPixel(CenterX - Y, CenterY + X);
			SetBrushPixel(CenterX + Y, CenterY + X);

			// NOTE(ariel) Account for motion along y-axis.
			D += dY;
			dY -= 4;
			Y += 1;

			// NOTE(ariel) Account for motion along x-axis. Shift D right by 31
			// bits to use its sign bit essentially as a boolean, adjusting values
			// along x if and only if D is less than zero.
			b32 LessThanZeroMask = D >> 31;
			D += dX & LessThanZeroMask;
			dX -= 4 & LessThanZeroMask;
			X += LessThanZeroMask;
		}
	}
}

int
main(void)
{
	SeedRandom();

	PlatformOpenWindow();
	renderer_context RendererContext = {0};
	InitializeRenderer(&RendererContext);

	u64 FrameCount = 0;
	u64 CurrentTimestamp = 0;
	u64 PreviousTimestamp = 0;
	u64 DeltaTime = 0;

	BoundCells();

	while (Running)
	{
		CurrentTimestamp = PlatformGetTime();

		PlatformHandleInput();

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

			FinishMenu();
		}

		// NOTE(ariel) Map new input in window coordinates to cell space.
		if (ShouldSpawnCells())
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
				Cell(X, Y).Type &= ~UPDATED;
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

		PresentBuffer();

		QuadsCount = 0;
		FrameCount += 1;

		DeltaTime = CurrentTimestamp - PreviousTimestamp;
		PreviousTimestamp = CurrentTimestamp;
		PlatformSleep(DeltaTime);
	}

	TerminateRenderer(RendererContext);
	PlatformCloseWindow();
	return 0;
}
