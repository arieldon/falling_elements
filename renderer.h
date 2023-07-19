#ifndef RENDERER_H
#define RENDERER_H

enum { FRAMES_PER_SECOND = 60 };

typedef struct { s32 X, Y; } Vector2s;

static u32 SandBufferIndex;
static u32 SandBuffers[2][WINDOW_WIDTH * WINDOW_HEIGHT];
static u32 Framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT];
static b32 Running = true;

static void ClearBuffer(void);
static void PresentBuffer(void);
static inline f64 GetTime(void);

#endif
