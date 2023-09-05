#ifndef AUTOMATA_H
#define AUTOMATA_H

enum
{
	CELL_SIZE = 4,
	CELL_START = 1,
	Y_CELL_COUNT = WINDOW_HEIGHT / CELL_SIZE,
	X_CELL_COUNT = WINDOW_WIDTH / CELL_SIZE,
	Y_CELL_COUNT_WITH_PADDING = Y_CELL_COUNT + 2,
	X_CELL_COUNT_WITH_PADDING = X_CELL_COUNT + 2,
};

enum { UPDATED = 0x80 };

// TODO(ariel) Make gas flammable?
typedef enum cell_type cell_type;
enum cell_type
{
	// NOTE(ariel) Sort cell type by density.
	BLANK,
	FIRE,
	GAS,
	WATER,
	SAND,
	WOOD,
	HOLY_BOUNDARY,
	CELL_TYPE_COUNT,

	UPDATED_GAS = GAS | UPDATED,
	UPDATED_FIRE = FIRE | UPDATED,
} __attribute__((packed));
StaticAssert(sizeof(cell_type) == 1);

u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[GAS] = 0xff333333,
	[WATER] = 0xffcc0000,
	[SAND] = 0xff00ccff,
	[WOOD] = 0xff38495c,
	[FIRE] = 0xff3344ff,
	[HOLY_BOUNDARY] = 0xffffffff,
};

typedef struct cell cell;
struct cell
{
	cell_type Type;
	u8 ColorModification;
	union
	{
		u16 FramesToLive;
		u16 Speed;
	};
};
StaticAssert(sizeof(cell) == 4);

static cell CellBuffer[Y_CELL_COUNT_WITH_PADDING * X_CELL_COUNT_WITH_PADDING];

#define Cell(X, Y) CellBuffer[(X_CELL_COUNT_WITH_PADDING)*(Y) + (X)]

static void TransitionCell(s32 X, s32 Y);
static void TransitionGasCell(s32 X, s32 Y);
static void TransitionWaterCell(s32 X, s32 Y);
static void TransitionSandCell(s32 X, s32 Y);
static void TransitionFireCell(s32 X, s32 Y);

static void SpawnCells(s32 X, s32 Y);
static void CreateBoundary(void);

#endif
