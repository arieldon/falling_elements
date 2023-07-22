#ifndef RENDERER_H
#define RENDERER_H

enum { FRAMES_PER_SECOND = 60 };
enum { YELLOW = 0xffff00 };

typedef struct { s32 X, Y; } Vector2s;

enum
{
	Y_CELL_COUNT = WINDOW_HEIGHT / 5,
	X_CELL_COUNT = WINDOW_WIDTH / 5,
};
static s32 ActiveSandBufferIndex;
static u32 SandBuffers[2][Y_CELL_COUNT * X_CELL_COUNT];

static u32 Framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT];
static b32 Running = true;

static void ClearBuffer(void);
static void PresentBuffer(void);
static inline f64 GetTime(void);

#endif
