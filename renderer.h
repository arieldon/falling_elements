#ifndef RENDERER_H
#define RENDERER_H

enum { TARGET_FRAMES_PER_SECOND = 60 };
static const u64 NANOSECONDS_PER_SECOND = 1000000000l;
static const u64 MILLISECONDS_PER_SECOND = 1000l;

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
	s32 Width;
	s32 Height;
	u32 Color;
	u8 TextureID;
};

typedef struct renderer_context renderer_context;
struct renderer_context
{
	GLuint ShaderProgram;
	GLuint Texture;
	GLuint VertexArray;
	GLuint InstancesBuffer;
	GLuint BaseVerticesBuffer;
};

static void InitializeRenderer(renderer_context *Context);
static void TerminateRenderer(renderer_context Context);

static void PresentBuffer(void);

#endif
