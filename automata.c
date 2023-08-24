u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[WATER] = 0xffcc0000,
	[SAND] = 0xff00ccff,
	[WOOD] = 0xff38495c,
	[HOLY_BOUNDARY] = 0xffffffff,
};

static void
TransitionCell(s32 X, s32 Y)
{
	switch (Cell(X, Y).Type)
	{
		case BLANK: break;
		case WATER: TransitionWaterCell(X, Y); break;
		case SAND: TransitionSandCell(X, Y); break;
		case WOOD: break;
		case HOLY_BOUNDARY: break;
		case CELL_TYPE_COUNT: break;
	}
}

static void
TransitionWaterCell(s32 X, s32 Y)
{
	u32 Direction = RandomU32InRange(0, 1) ? -1 : 1;
	if (Cell(X, Y+1).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X, Y+1));
	}
	else if (Cell(X-Direction, Y).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X-Direction, Y));
	}
	else if (Cell(X+Direction, Y).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X+Direction, Y));
	}
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	if (Cell(X, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X, Y+1));
	}
	else if (Cell(X-1, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X-1, Y+1));
	}
	else if (Cell(X+1, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X+1, Y+1));
	}
}

static void
CreateBoundary(void)
{
	for (s32 X = 0; X < X_CELL_COUNT; X += 1)
	{
		Cell(X, 0).Type = HOLY_BOUNDARY;
		Cell(X, 0).Color = CellTypeColorTable[HOLY_BOUNDARY];
		Cell(X, Y_CELL_COUNT).Type = HOLY_BOUNDARY;
		Cell(X, Y_CELL_COUNT).Color = CellTypeColorTable[HOLY_BOUNDARY];
	}
	for (s32 Y = 0; Y < Y_CELL_COUNT; Y += 1)
	{
		Cell(0, Y).Type = HOLY_BOUNDARY;
		Cell(0, Y).Color = CellTypeColorTable[HOLY_BOUNDARY];
		Cell(X_CELL_COUNT, Y).Type = HOLY_BOUNDARY;
		Cell(X_CELL_COUNT, Y).Color = CellTypeColorTable[HOLY_BOUNDARY];
	}
}
