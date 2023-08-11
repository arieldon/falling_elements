#ifndef RENDERER_H
#define RENDERER_H

enum { TARGET_FRAMES_PER_SECOND = 60 };
static const u64 NANOSECONDS_PER_SECOND = 1000000000l;

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

enum
{
	CELL_SIZE = 8,
	HALF_CELL_SIZE = CELL_SIZE / 2,
	Y_CELL_COUNT = WINDOW_HEIGHT / CELL_SIZE,
	X_CELL_COUNT = WINDOW_WIDTH / CELL_SIZE,
};

typedef enum cell_type cell_type;
enum cell_type
{
	BLANK,
	WOOD,
	SAND,
	WATER,
	CELL_TYPE_COUNT,
} __attribute__((packed));
StaticAssert(sizeof(cell_type) == 1);

typedef struct cell cell;
struct cell
{
	u32 Color;
	cell_type Type;
};

static cell CellBuffer[Y_CELL_COUNT * X_CELL_COUNT];

static s32 QuadsCount;
static quad Quads[ArrayCount(CellBuffer)];

typedef struct renderer_context renderer_context;
struct renderer_context
{
	GLuint ShaderProgram;
	GLuint VertexArray;
	GLuint InstancesBuffer;
	GLuint BaseVerticesBuffer;
};

static void InitializeRenderer(renderer_context *Context);
static void TerminateRenderer(renderer_context Context);

static void PresentBuffer(void);

static inline u64 GetTime(void);
static inline void SleepForNanoseconds(u64 DeltaTimeNS);

#endif
