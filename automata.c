static void
TransitionCell(s32 X, s32 Y)
{
	switch (Cell(X, Y).Type)
	{
		case SMOKE:
		case GAS: TransitionGasCell(X, Y); break;
		case FIRE: TransitionFireCell(X, Y); break;
		case WATER: TransitionWaterCell(X, Y); break;
		case SAND: TransitionSandCell(X, Y); break;
		default: break;
	}
}

static inline s32
GetDirection(void)
{
	s32 Result = RandomU32InRange(0, 1) * 2 - 1;
	Assert(Result == 1 | Result == -1);
	return Result;
}

static inline s16
AddSpeedSaturated(s16 Speed)
{
	s16 Result = Min(8, Speed+1);
	return Result;
}

static void
TransitionGasCell(s32 X, s32 Y)
{
	s32 SwapX = X;
	s32 SwapY = Y;
	s32 Speed = 0;
	s32 Direction = GetDirection();

	if (!Cell(X, Y).FramesToLive)
	{
		Cell(X, Y).Type = BLANK;
	}
	else
	{
		Speed = Min(4, 1+Min(X_CELL_COUNT-X, Y));
		for (s32 S = 1; S <= Speed; S += 1)
		{
			s32 Y0 = Y-S;
			s32 X1 = X+S*Direction;
			s32 X2 = X-S*Direction;

			b32 A = Cell(X, Y0).Type < GAS;
			b32 B = Cell(X1, Y).Type < GAS;
			b32 C = Cell(X2, Y).Type < GAS;

			SwapX = C ? X2 : SwapX;
			SwapY = C ? Y : SwapY;
			SwapX = B ? X1 : SwapX;
			SwapY = B ? Y : SwapY;
			SwapX = A ? X : SwapX;
			SwapY = A ? Y0 : SwapY;

			Speed *= A & B & C;
		}
	}

	Cell(X, Y).FramesToLive -= 1;
	Cell(X, Y).Type |= UPDATED;
	Swap(Cell(X, Y), Cell(SwapX, SwapY));
}

static void
TransitionFireCell(s32 X, s32 Y)
{
	enum
	{
		SMOKE_COLOR_MODIFICAITON = 0x03,
		MAXIMUM_FIRE_SPEED = 4,
	};

	typedef enum fire_action fire_action;
	enum fire_action
	{
		FIRE_ACTION_NONE = 0 << 0,
		FIRE_ACTION_SPREAD = 1 << 0,
		FIRE_ACTION_EVAPORATE = 1 << 1,
		FIRE_ACTION_MOVE = 1 << 2,
	};
	fire_action FireAction = FIRE_ACTION_NONE;

	if (!Cell(X, Y).FramesToLive)
	{
		Cell(X, Y).Type = BLANK;
	}
	else
	{
		s32 SwapX = X;
		s32 SwapY = Y;
		s32 Speed = Min(MAXIMUM_FIRE_SPEED, 1+Min(X_CELL_COUNT-X, Y_CELL_COUNT-Y));
		s32 Direction = GetDirection();

		for (s32 S = 1, D = S*Direction; S <= Speed; S += 1, D = S*Direction)
		{
			if (Cell(X+0, Y+S).Type == WOOD | Cell(X+0, Y+S).Type == GAS)
			{
				SwapX = X+0;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X-D, Y+S).Type == WOOD | Cell(X-D, Y+S).Type == GAS)
			{
				SwapX = X-D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X+D, Y+S).Type == WOOD | Cell(X+D, Y+S).Type == GAS)
			{
				SwapX = X+D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X-D, Y+0).Type == WOOD | Cell(X-D, Y+0).Type == GAS)
			{
				SwapX = X-D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X+D, Y+0).Type == WOOD | Cell(X+D, Y+0).Type == GAS)
			{
				SwapX = X+D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X+0, Y-S).Type == WOOD | Cell(X+0, Y-S).Type == GAS)
			{
				SwapX = X+0;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X-D, Y-S).Type == WOOD | Cell(X-D, Y-S).Type == GAS)
			{
				SwapX = X-D;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if (Cell(X+D, Y-S).Type == WOOD | Cell(X+D, Y-S).Type == GAS)
			{
				SwapX = X+D;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_SPREAD;
			}
			else if ((Cell(X+0, Y+S).Type & ~UPDATED) == WATER)
			{
				SwapX = X+0;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X-D, Y+S).Type & ~UPDATED) == WATER)
			{
				SwapX = X-D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X+D, Y+S).Type & ~UPDATED) == WATER)
			{
				SwapX = X+D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X-D, Y+0).Type & ~UPDATED) == WATER)
			{
				SwapX = X-D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X+D, Y+0).Type & ~UPDATED) == WATER)
			{
				SwapX = X+D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X+0, Y-S).Type & ~UPDATED) == WATER)
			{
				SwapX = X+0;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X-D, Y-S).Type & ~UPDATED) == WATER)
			{
				SwapX = X-D;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if ((Cell(X+D, Y-S).Type & ~UPDATED) == WATER)
			{
				SwapX = X+D;
				SwapY = Y-S;
				FireAction = FIRE_ACTION_EVAPORATE;
			}
			else if (Cell(X+0, Y+S).Type < FIRE)
			{
				SwapX = X;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_MOVE;
			}
			else if (Cell(X-D, Y+S).Type < FIRE)
			{
				SwapX = X-D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_MOVE;
			}
			else if (Cell(X+D, Y+S).Type < FIRE)
			{
				SwapX = X+D;
				SwapY = Y+S;
				FireAction = FIRE_ACTION_MOVE;
			}
			else if (Cell(X-D, Y+0).Type < FIRE)
			{
				SwapX = X-D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_MOVE;
			}
			else if (Cell(X+D, Y+0).Type < FIRE)
			{
				SwapX = X+D;
				SwapY = Y+0;
				FireAction = FIRE_ACTION_MOVE;
			}
			else
			{
				break;
			}
		}

		switch (FireAction)
		{
			case FIRE_ACTION_NONE:
			{
				Cell(X, Y).FramesToLive -= 1;
				Cell(X, Y).Type = UPDATED_FIRE;
				break;
			}
			case FIRE_ACTION_SPREAD:
			{
				Cell(X, Y).FramesToLive += 2;
				Cell(X, Y).Type = Cell(SwapX, SwapY).Type = UPDATED_FIRE;
				Cell(SwapX, SwapY).FramesToLive = 256;
				break;
			}
			case FIRE_ACTION_EVAPORATE:
			{
				Cell(X, Y).Type = UPDATED_SMOKE;
				Cell(X, Y).ColorModification = (u8)RandomU32InRange(0x00, SMOKE_COLOR_MODIFICAITON);
				Cell(X, Y).FramesToLive = 512;
				Cell(SwapX, SwapY).Type = BLANK;
				break;
			}
			case FIRE_ACTION_MOVE:
			{
				Cell(X, Y).FramesToLive -= 1;
				Cell(X, Y).Type |= UPDATED;
				Swap(Cell(X, Y), Cell(SwapX, SwapY));
				break;
			}
		}
	}
}

static void
TransitionWaterCell(s32 X, s32 Y)
{
	s32 SwapX = X;
	s32 SwapY = Y;
	s32 Speed = 0;
	s32 Direction = GetDirection();

	Speed = Min(Cell(X, Y).Speed, 1+Min(X_CELL_COUNT-X, Y_CELL_COUNT-Y));
	for (s32 S = 1; S <= Speed; S += 1)
	{
		s32 Y0 = Y+S;
		b32 A = (s8)Cell(X, Y0).Type < WATER;
		SwapY = A ? Y0 : SwapY;
		Speed *= A;
	}

	Speed = (SwapY == Y) * Min(4, 1+Min(X_CELL_COUNT-X, Y_CELL_COUNT-Y));
	for (s32 S = 1; S <= Speed; S += 1)
	{
		s32 X1 = X+S*Direction;
		s32 X2 = X-S*Direction;

		b32 B = (s8)Cell(X1, Y).Type < WATER;
		b32 C = (s8)Cell(X2, Y).Type < WATER;
		SwapX = C ? X2 : SwapX;
		SwapX = B ? X1 : SwapX;

		Speed *= B & C;
	}

	Cell(X, Y).Speed = AddSpeedSaturated(Cell(X, Y).Speed);
	Swap(Cell(X, Y), Cell(SwapX, SwapY));
}

static void
TransitionSandCell(s32 X, s32 Y)
{
	s32 SwapX = X;
	s32 SwapY = Y;
	s32 Speed = Min(Cell(X, Y).Speed, 1+Min(X_CELL_COUNT-X, Y_CELL_COUNT-Y));

	for (s32 S = 1; S <= Speed; S += 1)
	{
		s32 YN = Y+S;
		s32 X0 = X+0;
		s32 X1 = X-S;
		s32 X2 = X+S;

		// NOTE(ariel) Is there a way to specify default signedness of an enum upon
		// definition?
		b32 A = (s8)Cell(X0, YN).Type < SAND;
		b32 B = (s8)Cell(X1, YN).Type < SAND;
		b32 C = (s8)Cell(X2, YN).Type < SAND;

		SwapY = A | B | C ? YN : SwapY;
		SwapX = C ? X2 : SwapX;
		SwapX = B ? X1 : SwapX;
		SwapX = A ? X0 : SwapX;

		// NOTE(ariel) Some obstruction (more dense cell) may block this cell from
		// further movement.
		Speed *= A | B | C;
	}

	Cell(X, Y).Speed = AddSpeedSaturated(Cell(X, Y).Speed);
	Swap(Cell(X, Y), Cell(SwapX, SwapY));
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

	s32 Radius = 8;
	s32 RadiusSquared = Radius*Radius;
	for (s32 CircleY = -Radius; CircleY <= Radius; CircleY += 1)
	{
		for (s32 CircleX = -Radius; CircleX <= Radius; CircleX += 1)
		{
			if (CircleY*CircleY + CircleX*CircleX <= RadiusSquared)
			{
				u32 CellY = Clamp(CircleY+Y, 0, Y_CELL_COUNT);
				u32 CellX = Clamp(CircleX+X, 0, X_CELL_COUNT);
				if (Creating == BLANK | Cell(CellX, CellY).Type == BLANK)
				{
					b32 Chance = Creating == WOOD | RandomU32InRange(0, 4) == 0;
					cell_type NewType = (cell_type)(Creating * Chance);
					Cell(CellX, CellY).Type = NewType;
					Cell(CellX, CellY).ColorModification = (u8)RandomU32InRange(0x00, ColorModifications[Creating]);
					Cell(CellX, CellY).Speed = 1 + 511*(Creating == GAS | Creating == FIRE);
				}
			}
		}
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
