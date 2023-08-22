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
	// NOTE(ariel) Because all cells share the same width and height, these
	// fields are specified implicitly here and set in the shader.
	s32 X;
	s32 Y;
	u32 Color;
};

typedef struct renderer_context renderer_context;
struct renderer_context
{
	GLuint ShaderProgram;
	GLint UniformScaleLocation;
	GLuint VertexArray;
	GLuint InstancesBuffer;
	GLuint BaseVerticesBuffer;
};

static void InitializeRenderer(renderer_context *Context);
static void TerminateRenderer(renderer_context Context);

static void PresentBuffer(renderer_context Context);

static inline u64 GetTime(void);
static inline void SleepForNanoseconds(u64 DeltaTimeNS);

#endif
