#ifndef AUTOMATA_H
#define AUTOMATA_H

enum
{
	CELL_SIZE = 8,
	HALF_CELL_SIZE = CELL_SIZE / 2,
	CELL_START = 1,
	Y_CELL_COUNT = WINDOW_HEIGHT / CELL_SIZE,
	X_CELL_COUNT = WINDOW_WIDTH / CELL_SIZE,
};

typedef enum cell_type cell_type;
enum cell_type
{
	// NOTE(ariel) Sort cell type by density.
	BLANK,
	WATER,
	SAND,
	WOOD,
	HOLY_BOUNDARY,
	CELL_TYPE_COUNT,
} __attribute__((packed));
StaticAssert(sizeof(cell_type) == 1);

typedef struct cell cell;
struct cell
{
	u32 Color;
	cell_type Type;
};

static cell CellBuffer[(Y_CELL_COUNT+1) * (X_CELL_COUNT+1)];

#define Cell(X, Y) CellBuffer[(X_CELL_COUNT)*(Y) + (X)]

static void TransitionCell(s32 X, s32 Y);
static void TransitionWaterCell(s32 X, s32 Y);
static void TransitionSandCell(s32 X, s32 Y);
static void CreateBoundary(void);

#endif
