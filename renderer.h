#ifndef RENDERER_H
#define RENDERER_H

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

typedef struct { s32 X, Y; } Vector2s;

enum
{
	Y_CELL_COUNT = WINDOW_HEIGHT / 5,
	X_CELL_COUNT = WINDOW_WIDTH / 5,
};
static s32 ActiveCellBufferIndex;
static cell_type CellBuffers[2][Y_CELL_COUNT * X_CELL_COUNT];

static u32 Framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT];
static b32 Running = true;

static void ClearBuffer(void);
static void PresentBuffer(void);
static inline u64 GetTime(void);
static inline void SleepForNanoseconds(u64 DeltaTimeNS);

#endif
