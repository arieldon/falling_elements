#ifndef AUTOMATA_H
#define AUTOMATA_H

enum
{
	CELL_SIZE = 8,
	CELL_START = 1,
	Y_CELL_COUNT = WINDOW_HEIGHT / CELL_SIZE,
	X_CELL_COUNT = WINDOW_WIDTH / CELL_SIZE,
	Y_CELL_COUNT_WITH_PADDING = Y_CELL_COUNT + 2,
	X_CELL_COUNT_WITH_PADDING = X_CELL_COUNT + 2,
};

typedef enum cell_type cell_type;
enum cell_type
{
	// NOTE(ariel) Sort cell type by density.
	BLANK,
	GAS,
	WATER,
	SAND,
	WOOD,
	HOLY_BOUNDARY,
	CELL_TYPE_COUNT,
} __attribute__((packed));
StaticAssert(sizeof(cell_type) == 1);

u32 CellTypeColorTable[CELL_TYPE_COUNT] =
{
	[BLANK] = 0x00000000,
	[WATER] = 0xffcc0000,
	[GAS] = 0xff00cc00,
	[SAND] = 0xff00ccff,
	[WOOD] = 0xff38495c,
	[HOLY_BOUNDARY] = 0xffffffff,
};

typedef struct cell cell;
struct cell
{
	cell_type Type;
	u8 Updated;
	u8 ColorModification;
};

static cell CellBuffer[Y_CELL_COUNT_WITH_PADDING * X_CELL_COUNT_WITH_PADDING];

#define Cell(X, Y) CellBuffer[(X_CELL_COUNT_WITH_PADDING)*(Y) + (X)]

static void TransitionCell(s32 X, s32 Y);
static void TransitionGasCell(s32 X, s32 Y);
static void TransitionWaterCell(s32 X, s32 Y);
static void TransitionSandCell(s32 X, s32 Y);
static void CreateBoundary(void);

#endif
