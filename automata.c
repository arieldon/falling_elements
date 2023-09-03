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
			case FIRE: TransitionFireCell(X, Y); break;
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
	Cell(X, Y).FramesToLive -= 1;

	s32 Direction = GetDirection();
	if (Cell(X, Y).FramesToLive == 0)
	{
		Cell(X, Y).Type = BLANK;
		Cell(X, Y).Updated = true;
		Cell(X, Y).ColorModification = 0x00;
	}
	else if (Cell(X, Y-1).Type < GAS)
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
TransitionFireCell(s32 X, s32 Y)
{
	enum { SMOKE_COLOR_MODIFICAITON = 0x03 };

	Cell(X, Y).FramesToLive -= 1;

	s32 Direction = GetDirection();
	if (Cell(X, Y).FramesToLive == 0)
	{
		Cell(X, Y).Type = BLANK;
		Cell(X, Y).Updated = true;
		Cell(X, Y).ColorModification = 0x00;
	}
	else if (Cell(X, Y+1).Type == WOOD)
	{
		Cell(X, Y+1).Type = FIRE;
		Cell(X, Y+1).FramesToLive += 2;
		Cell(X, Y+1).Updated = true;
	}
	else if (Cell(X-Direction, Y).Type == WOOD)
	{
		Cell(X-Direction, Y).Type = FIRE;
		Cell(X-Direction, Y).FramesToLive += 2;
		Cell(X-Direction, Y).Updated = Cell(X, Y).Updated = true;
	}
	else if (Cell(X+Direction, Y).Type == WOOD)
	{
		Cell(X+Direction, Y).Type = FIRE;
		Cell(X+Direction, Y).FramesToLive += 2;
		Cell(X+Direction, Y).Updated = Cell(X, Y).Updated = true;
	}
	else if (Cell(X, Y+1).Type == WATER)
	{
		Cell(X, Y).Type = GAS;
		Cell(X, Y).ColorModification = (u8)RandomU32InRange(0x00, SMOKE_COLOR_MODIFICAITON);
		Cell(X, Y).FramesToLive = 255;
		Cell(X, Y).Updated = true;
		Cell(X, Y+1).Type = BLANK;
		Cell(X, Y+1).Updated = true;
		Cell(X, Y+1).ColorModification = 0x00;
	}
	else if (Cell(X-Direction, Y).Type == WATER)
	{
		Cell(X, Y).Type = GAS;
		Cell(X, Y).ColorModification = (u8)RandomU32InRange(0x00, SMOKE_COLOR_MODIFICAITON);
		Cell(X, Y).FramesToLive = 255;
		Cell(X, Y).Updated = true;
		Cell(X-Direction, Y).Type = BLANK;
		Cell(X-Direction, Y).Updated = true;
		Cell(X-Direction, Y).ColorModification = 0x00;
	}
	else if (Cell(X+Direction, Y).Type == WATER)
	{
		Cell(X, Y).Type = GAS;
		Cell(X, Y).ColorModification = (u8)RandomU32InRange(0x00, SMOKE_COLOR_MODIFICAITON);
		Cell(X, Y).FramesToLive = 255;
		Cell(X, Y).Updated = true;
		Cell(X+Direction, Y).Type = BLANK;
		Cell(X+Direction, Y).Updated = true;
		Cell(X+Direction, Y).ColorModification = 0x00;
	}
	else if (Cell(X, Y+1).Type <= GAS)
	{
		Swap(Cell(X, Y), Cell(X, Y+1));
		Cell(X, Y).Updated = Cell(X+1, Y+1).Updated = true;
	}
	else if (Cell(X-Direction, Y).Type <= GAS)
	{
		Swap(Cell(X, Y), Cell(X-Direction, Y));
		Cell(X, Y).Updated = Cell(X-Direction, Y).Updated = true;
	}
	else if (Cell(X+Direction, Y).Type <= GAS)
	{
		Swap(Cell(X, Y), Cell(X+Direction, Y));
		Cell(X, Y).Updated = Cell(X+Direction, Y).Updated = true;
	}

	// NOTE(ariel) Spawn smoke above with random chance.
	if (Cell(X, Y-1).Type == BLANK)
	{
		u8 Chance = RandomU32InRange(0, 128) == 0;
		Cell(X, Y-1).Type = GAS * Chance;
		Cell(X, Y-1).ColorModification = (u8)RandomU32InRange(0x00, SMOKE_COLOR_MODIFICAITON);
		Cell(X, Y-1).FramesToLive = 16;
		Cell(X, Y-1).Updated = true;
	}
}

static void
SpawnCells(s32 X, s32 Y)
{
	static u8 ColorModifications[CELL_TYPE_COUNT] =
	{
		[GAS] = 0xcc,
		[WATER] = 0x80,
		[SAND] = 0x33,
		[WOOD] = 0x22,
		[FIRE] = 0x80,
	};

	if (Creating != BLANK)
	{
		s32 Radius = 4;
		s32 RadiusSquared = Radius*Radius;
		for (s32 CircleY = -Radius; CircleY <= Radius; CircleY += 1)
		{
			for (s32 CircleX = -Radius; CircleX <= Radius; CircleX += 1)
			{
				if (CircleY*CircleY + CircleX*CircleX <= RadiusSquared)
				{
					u32 CellY = Clamp(CircleY+Y, 0, Y_CELL_COUNT);
					u32 CellX = Clamp(CircleX+X, 0, X_CELL_COUNT);
					if (Cell(CellX, CellY).Type == BLANK)
					{
						b32 Chance = Creating == WOOD || RandomU32InRange(0, 4) == 0;
						cell_type NewType = (cell_type)(Creating * Chance);
						Cell(CellX, CellY).Type = NewType;
						Cell(CellX, CellY).ColorModification = (u8)RandomU32InRange(0x00, ColorModifications[Creating]);
						Cell(CellX, CellY).FramesToLive = 128;
					}
				}
			}
		}
	}
	else
	{
		// NOTE(ariel) Spawn blanks to erase.
		Cell(X, Y).Type = Creating;
		Cell(X, Y).ColorModification = 0x00;
	}
}

static void
BoundCells(void)
{
	for (s32 X = 0; X < X_CELL_COUNT_WITH_PADDING; X += 1)
	{
		Cell(X, 0).Type = HOLY_BOUNDARY;
		Cell(X, 0).ColorModification = 0x00;
		Cell(X, Y_CELL_COUNT+1).Type = HOLY_BOUNDARY;
		Cell(X, Y_CELL_COUNT+1).ColorModification = 0x00;
	}
	for (s32 Y = 0; Y < Y_CELL_COUNT_WITH_PADDING; Y += 1)
	{
		Cell(0, Y).Type = HOLY_BOUNDARY;
		Cell(0, Y).ColorModification = 0x00;
		Cell(X_CELL_COUNT+1, Y).Type = HOLY_BOUNDARY;
		Cell(X_CELL_COUNT+1, Y).ColorModification = 0x00;
	}
}
