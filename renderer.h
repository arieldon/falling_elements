#ifndef RENDERER_H
#define RENDERER_H

// TODO(ariel) Should I use floating points to define colors instead of
// integers? Color really doesn't matter much in my program. I don't need
// interpolation or anything like that. At least I don't think I do as of now.
enum { TARGET_FRAMES_PER_SECOND = 60 };
static const u64 NANOSECONDS_PER_SECOND = 1000000000l;

typedef enum cell_type cell_type;
enum cell_type
{
	BLANK,
	SAND,
	WATER,
	CELL_TYPE_COUNT,
} __attribute__((packed));
StaticAssert(sizeof(cell_type) == 1);

typedef struct vector2s vector2s;
struct vector2s
{
	s32 X;
	s32 Y;
};

typedef struct quad quad;
struct quad
{
	s32 X;
	s32 Y;
	u32 Color;
};

typedef struct circle circle;
struct circle
{
	s32 Radius;
	vector2s Center;
};

enum
{
	CELL_SIZE = 8,
	HALF_CELL_SIZE = CELL_SIZE / 2,
	Y_CELL_COUNT = WINDOW_HEIGHT / CELL_SIZE,
	X_CELL_COUNT = WINDOW_WIDTH / CELL_SIZE,
};
static cell_type CellBuffer[Y_CELL_COUNT * X_CELL_COUNT];

static s32 QuadsCount;
static quad Quads[ArrayCount(CellBuffer)];

static void InitializeRenderer(void);
static void PresentBuffer(void);

static inline u64 GetTime(void);
static inline void SleepForNanoseconds(u64 DeltaTimeNS);

#endif
