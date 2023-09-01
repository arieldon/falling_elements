u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[WATER] = 0xffcc0000,
	[GAS] = 0xff00cc00,
	[SAND] = 0xff00ccff,
	[WOOD] = 0xff38495c,
	[HOLY_BOUNDARY] = 0xffffffff,
};

static void
TransitionCell(s32 X, s32 Y)
{
	if (!Cell(X, Y).Updated)
	{
		switch (Cell(X, Y).Type)
		{
			case BLANK: break;
			case GAS: TransitionGasCell(X, Y); break;
			case WATER: TransitionWaterCell(X, Y); break;
			case SAND: TransitionSandCell(X, Y); break;
			case WOOD: break;
			case HOLY_BOUNDARY: break;
			case CELL_TYPE_COUNT: break;
		}
	}
}

static s32 inline
GetDirection(void)
{
	s32 Result = RandomU32InRange(0, 1) * 2 - 1;
	return Result;
}

static void
TransitionGasCell(s32 X, s32 Y)
{
	s32 Direction = GetDirection();
	if (Cell(X, Y-1).Type < GAS)
	{
		Swap(Cell(X, Y), Cell(X, Y-1));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X, Y-1).Updated = true;
	}
	else if (Cell(X-Direction, Y).Type < GAS)
	{
		Swap(Cell(X, Y), Cell(X-Direction, Y));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X-Direction, Y).Updated = true;
	}
	else if (Cell(X+Direction, Y).Type < GAS)
	{
		Swap(Cell(X, Y), Cell(X+Direction, Y));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X+Direction, Y).Updated = true;
	}
}

static void
TransitionWaterCell(s32 X, s32 Y)
{
	s32 Direction = GetDirection();
	if (Cell(X, Y+1).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X, Y+1));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X, Y+1).Updated = true;
	}
	else if (Cell(X-Direction, Y).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X-Direction, Y));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X-Direction, Y).Updated = true;
	}
	else if (Cell(X+Direction, Y).Type < WATER)
	{
		Swap(Cell(X, Y), Cell(X+Direction, Y));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X+Direction, Y).Updated = true;
	}
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	if (Cell(X, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X, Y+1));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X, Y+1).Updated = true;
	}
	else if (Cell(X-1, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X-1, Y+1));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X-1, Y+1).Updated = true;
	}
	else if (Cell(X+1, Y+1).Type < SAND)
	{
		Swap(Cell(X, Y), Cell(X+1, Y+1));
		Cell(X, Y).Updated = Cell(X, Y).Type == BLANK;
		Cell(X+1, Y+1).Updated = true;
	}
}

static void
BoundCells(void)
{
	for (s32 X = 0; X < X_CELL_COUNT_WITH_PADDING; X += 1)
	{
		Cell(X, 0).Type = HOLY_BOUNDARY;
		Cell(X, 0).Color = CellTypeColorTable[HOLY_BOUNDARY];
		Cell(X, Y_CELL_COUNT+1).Type = HOLY_BOUNDARY;
		Cell(X, Y_CELL_COUNT+1).Color = CellTypeColorTable[HOLY_BOUNDARY];
	}
	for (s32 Y = 0; Y < Y_CELL_COUNT_WITH_PADDING; Y += 1)
	{
		Cell(0, Y).Type = HOLY_BOUNDARY;
		Cell(0, Y).Color = CellTypeColorTable[HOLY_BOUNDARY];
		Cell(X_CELL_COUNT+1, Y).Type = HOLY_BOUNDARY;
		Cell(X_CELL_COUNT+1, Y).Color = CellTypeColorTable[HOLY_BOUNDARY];
	}
}
